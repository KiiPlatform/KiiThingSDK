/*
  kii_cloud.c
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#include "kii_custom.h"
#include "kii_http_adapter.h"
#include "kii_prv_utils.h"
#include "kii_prv_types.h"

kii_error_code_t kii_global_init(void)
{
    kii_bool_t r = kii_http_init();
    return ((r == KII_TRUE) ? KIIE_OK : KIIE_FAIL);
}

void kii_global_cleanup(void)
{
    kii_http_cleanup();
}

void kii_dispose_kii_char(kii_char_t* char_ptr)
{
    M_KII_FREE_NULLIFY(char_ptr);
}

prv_kii_thing_t* prv_kii_init_thing(const kii_char_t* kii_thing_id)
{
    prv_kii_thing_t* ret = kii_malloc(sizeof(prv_kii_thing_t));
    kii_char_t* thingIdCopy = NULL;
    if (ret == NULL) {
        return NULL;
    }
    thingIdCopy = kii_strdup(kii_thing_id);
    if (thingIdCopy == NULL) {
        M_KII_FREE_NULLIFY(ret);
        return NULL;
    }
    ret->kii_thing_id = thingIdCopy;
    return ret;
}

kii_app_t kii_init_app(const kii_char_t* app_id,
                       const kii_char_t* app_key,
                       const kii_char_t* site_url)
{
    prv_kii_app_t* app = kii_malloc(sizeof(prv_kii_app_t));
    if (app == NULL) {
        return app;
    }
    app->last_result = KIIE_OK;
    app->app_id = kii_strdup(app_id);
    if (app->app_id == NULL) {
        M_KII_FREE_NULLIFY(app);
        return app;
    }

    app->app_key = kii_strdup(app_key);
    if (app->app_key == NULL) {
        M_KII_FREE_NULLIFY(app->app_id);
        M_KII_FREE_NULLIFY(app);
        return app;
    }

    app->site_url = kii_strdup(site_url);
    if (app->site_url == NULL) {
        M_KII_FREE_NULLIFY(app->app_id);
        M_KII_FREE_NULLIFY(app->app_key);
        M_KII_FREE_NULLIFY(app);
        return app;
    }

    return app;
}

kii_error_t* kii_get_last_error(kii_app_t app)
{
    switch (app->last_result) {
        case KIIE_OK:
        case KIIE_LOWMEMORY:
        case KIIE_RESPWRITE:
        case KIIE_ADAPTER:
            return NULL;
        case KIIE_FAIL:
            return &(app->last_error);
        default:
            /* this is programming error. */
            M_KII_ASSERT(0);
            return NULL;
    }
}

static void prv_kii_set_info_in_error(
        kii_error_t* error,
        int status_code,
        const kii_char_t* error_code)
{
    size_t max_buffer_size =
        sizeof(error->error_code) / sizeof(error->error_code[0]);
    error->status_code = status_code;
    if (error_code == NULL) {
        error->error_code[0] = '\0';
    } else {
        kii_strncpy(error->error_code, error_code, max_buffer_size - 1);
        error->error_code[max_buffer_size - 1] = '\0';
    }
}

static void prv_kii_set_last_error(
        prv_kii_app_t* app,
        kii_error_code_t error_code,
        kii_error_t* new_error)
{
    M_KII_ASSERT(app != NULL);
    app->last_result = error_code;
    if (app != NULL) {
        prv_kii_set_info_in_error(&(app->last_error),
              new_error->status_code, new_error->error_code);
    }
}

void kii_dispose_app(kii_app_t app)
{
    M_KII_FREE_NULLIFY(app->app_id);
    M_KII_FREE_NULLIFY(app->app_key);
    M_KII_FREE_NULLIFY(app->site_url);
    M_KII_FREE_NULLIFY(app);
}

void kii_dispose_bucket(kii_bucket_t bucket)
{
    M_KII_FREE_NULLIFY(bucket->bucket_name);
    M_KII_FREE_NULLIFY(bucket->kii_thing_id);
    M_KII_FREE_NULLIFY(bucket);
}

void kii_dispose_topic(kii_topic_t topic)
{
    M_KII_FREE_NULLIFY(topic->topic_name);
    M_KII_FREE_NULLIFY(topic->kii_thing_id);
    M_KII_FREE_NULLIFY(topic);
}

void kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t* endpoint)
{
    M_KII_FREE_NULLIFY(endpoint->password);
    M_KII_FREE_NULLIFY(endpoint->username);
    M_KII_FREE_NULLIFY(endpoint->host);
    M_KII_FREE_NULLIFY(endpoint->topic);
    endpoint->port_tcp = 0;
    endpoint->port_ssl = 0;
    M_KII_FREE_NULLIFY(endpoint);
}

json_t* prv_create_common_header_json_object(
        const kii_app_t app,
        const kii_char_t* opt_access_token,
        const kii_char_t* opt_content_type)
{
    json_t* ret = NULL;
    kii_int_t json_set_result = 0;

    ret = json_object();
    if (ret == NULL) {
        return NULL;
    }

    json_set_result = 0;
    json_set_result |= json_object_set_new(ret, "x-kii-appid",
            json_string(app->app_id));
    json_set_result |= json_object_set_new(ret, "x-kii-appkey",
            json_string(app->app_key));
    if (opt_access_token != NULL) {
        kii_char_t tmp[1024];
        sprintf(tmp, "bearer %s", opt_access_token);
        json_set_result |= json_object_set_new(ret, "authorization",
                json_string(tmp));
    }
    if (opt_content_type != NULL) {
        json_set_result |= json_object_set_new(ret, "content-type",
                json_string(opt_content_type));
    }
    if (json_set_result != 0) {
        json_decref(ret);
        return NULL;
    }
    return ret;
}

kii_error_code_t prv_parse_response_error_code(
        kii_int_t response_status_code,
        const kii_char_t* response_body,
        kii_error_t* error)
{
    const kii_char_t* error_code = NULL;
    json_t* errJson = NULL;
    json_t* errorCodeJson = NULL;

    if (response_body != NULL) {
        json_error_t jErr;
        errJson = json_loads(response_body, 0, &jErr);
        if (errJson == NULL) {
            return KIIE_LOWMEMORY;
        }
    }
    errorCodeJson = json_object_get(errJson, "errorCode");
    if (errorCodeJson != NULL) {
        error_code = json_string_value(errorCodeJson);
    } else {
        error_code = response_body;
    }
    prv_kii_set_info_in_error(error, response_status_code, error_code);
    json_decref(errJson);
    return KIIE_FAIL;
}

void kii_dispose_thing(kii_thing_t thing)
{
    if (thing != NULL) {
        M_KII_FREE_NULLIFY(thing->kii_thing_id);
    }
    M_KII_FREE_NULLIFY(thing);
}

kii_char_t* kii_thing_serialize(const kii_thing_t thing)
{
    kii_char_t* ret = NULL;

    M_KII_ASSERT(thing != NULL);
    M_KII_ASSERT(thing->kii_thing_id != NULL);
    M_KII_ASSERT(kii_strlen(thing->kii_thing_id) > 0);

    ret = kii_strdup(thing->kii_thing_id);
    return ret;
}

kii_thing_t kii_thing_deserialize(const kii_char_t* serialized_thing)
{
    M_KII_ASSERT(serialized_thing != NULL);
    M_KII_ASSERT(kii_strlen(serialized_thing) > 0);
    return (kii_thing_t) prv_kii_init_thing(serialized_thing);
}

static kii_error_code_t prv_prepare_register_thing_request_data(
        const kii_char_t* vendor_thing_id,
        const kii_char_t* thing_password,
        const kii_char_t* opt_thing_type,
        const json_t* user_data,
        kii_char_t** out_string)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* reqJson = NULL;
    kii_int_t json_set_result = 0;

    reqJson = (user_data == NULL) ? json_object() : json_deep_copy(user_data);
    if (reqJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    json_set_result = 0;
    json_set_result |= json_object_set_new(reqJson, "_vendorThingID",
            json_string(vendor_thing_id));
    json_set_result |= json_object_set_new(reqJson, "_password",
            json_string(thing_password));
    if (opt_thing_type != NULL && kii_strlen(opt_thing_type) > 0) {
        json_set_result |= json_object_set_new(reqJson, "_thingType",
                json_string(opt_thing_type));
    }
    if (json_set_result != 0) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    *out_string = json_dumps(reqJson, 0);
    ret = (*out_string == NULL) ? KIIE_LOWMEMORY : KIIE_OK;

ON_EXIT:
    json_decref(reqJson);

    return ret;
}

static kii_error_code_t
prv_prepare_authenticate_thing_request_data(const kii_char_t* vendor_thing_id,
                                            const kii_char_t* thing_password,
                                            kii_char_t** out_string)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* reqJson = NULL;
    kii_int_t json_set_result = 0;
    kii_char_t* qualified_thing_id = NULL;
    kii_char_t* prefix = "VENDOR_THING_ID:";
    size_t qualified_thing_id_size;

    qualified_thing_id_size =
        kii_strlen(vendor_thing_id) + kii_strlen(prefix) + 1;
    qualified_thing_id =
        kii_malloc(kii_strlen(vendor_thing_id) + kii_strlen(prefix) + 1);
    if (qualified_thing_id == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    qualified_thing_id[qualified_thing_id_size-1] = '\0';
    kii_strncat(qualified_thing_id, prefix, qualified_thing_id_size);
    kii_strncat(qualified_thing_id, vendor_thing_id, qualified_thing_id_size);

    reqJson = json_object();
    if (reqJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    json_set_result = 0;
    json_set_result |= json_object_set_new(reqJson, "username",
                                           json_string(qualified_thing_id));
    json_set_result |= json_object_set_new(reqJson, "password",
                                           json_string(thing_password));

    if (json_set_result != 0) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    *out_string = json_dumps(reqJson, 0);
    ret = (*out_string == NULL) ? KIIE_LOWMEMORY : KIIE_OK;
    
ON_EXIT:
    kii_free(qualified_thing_id);
    json_decref(reqJson);
    
    return ret;
}

static kii_error_code_t prv_parse_register_thing_response(
        kii_int_t respCode,
        const kii_char_t* respData,
        kii_thing_t* out_thing,
        kii_char_t** out_access_token,
        kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* respJson = NULL;
    json_error_t jErr;

    if (respCode < 200 || respCode >= 300) {
        ret = prv_parse_response_error_code(respCode, respData, err);
        goto ON_EXIT;
    }

    respJson = json_loads(respData, 0, &jErr);

    if (respJson == NULL) {
        ret = KIIE_LOWMEMORY;
    } else {
        const kii_char_t* accessToken = json_string_value(
                json_object_get(respJson, "_accessToken"));
        const kii_char_t* thingId = json_string_value(
                json_object_get(respJson, "_thingID"));
        if (accessToken != NULL && thingId != NULL) {
            ret = KIIE_OK;
            *out_access_token = kii_strdup(accessToken);
            *out_thing = (kii_thing_t)prv_kii_init_thing(thingId);
            if (*out_access_token == NULL || *out_thing == NULL) {
                M_KII_FREE_NULLIFY(*out_access_token);
                M_KII_FREE_NULLIFY(*out_thing);
                ret = KIIE_LOWMEMORY;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    }

ON_EXIT:
    json_decref(respJson);
    return ret;
}

kii_error_code_t kii_register_thing(kii_app_t app,
                                    const kii_char_t* vendor_thing_id,
                                    const kii_char_t* thing_password,
                                    const kii_char_t* opt_thing_type,
                                    const json_t* user_data,
                                    kii_thing_t* out_thing,
                                    kii_char_t** out_access_token)
{
    kii_char_t *reqUrl = NULL;
    json_t* headers = NULL;
    kii_char_t* reqStr = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(vendor_thing_id != NULL);
    M_KII_ASSERT(thing_password != NULL);
    M_KII_ASSERT(out_thing !=NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    /* prepare headers */
    headers = prv_create_common_header_json_object(app, NULL,
            "application/vnd.kii.ThingRegistrationAndAuthorizationRequest+json");
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    /* prepare request data */
    ret = prv_prepare_register_thing_request_data(vendor_thing_id,
            thing_password, opt_thing_type, user_data, &reqStr);
    if (ret != KIIE_OK) {
        goto ON_EXIT;
    }
    if (kii_http_execute("POST", reqUrl, headers, reqStr, &respCode, NULL,
                    &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT;
    }

    ret = prv_parse_register_thing_response(respCode, respData, out_thing,
            out_access_token, &err);
ON_EXIT:
    json_decref(headers);
    M_KII_FREE_NULLIFY(reqStr);
    M_KII_FREE_NULLIFY(respData);
    M_KII_FREE_NULLIFY(reqUrl);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

static kii_error_code_t
prv_parse_authenticate_thing_response(
                                      kii_int_t respCode,
                                      const kii_char_t* respData,
                                      kii_thing_t* out_thing,
                                      kii_char_t** out_access_token,
                                      kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* respJson = NULL;
    json_error_t jErr;

    if (respCode < 200 || respCode >= 300) {
        ret = prv_parse_response_error_code(respCode, respData, err);
        goto ON_EXIT;
    }

    respJson = json_loads(respData, 0, &jErr);

    if (respJson == NULL) {
        ret = KIIE_LOWMEMORY;
    } else {
        const kii_char_t* accessToken =
            json_string_value(json_object_get(respJson, "access_token"));
        const kii_char_t* thingId =
            json_string_value(json_object_get(respJson, "id"));
        if (accessToken != NULL && thingId != NULL) {
            ret = KIIE_OK;
            *out_access_token = kii_strdup(accessToken);
            *out_thing = (kii_thing_t)prv_kii_init_thing(thingId);
            if (*out_access_token == NULL || *out_thing == NULL) {
                M_KII_FREE_NULLIFY(*out_access_token);
                M_KII_FREE_NULLIFY(*out_thing);
                ret = KIIE_LOWMEMORY;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    }
ON_EXIT:
    json_decref(respJson);
    return ret;
}

kii_error_code_t kii_authenticate_thing(kii_app_t app,
                                        const kii_char_t* vendor_thing_id,
                                        const kii_char_t* thing_password,
                                        kii_thing_t* out_thing,
                                        kii_char_t** out_access_token)
{
    kii_char_t *reqUrl = NULL;
    json_t* headers = NULL;
    kii_char_t* reqStr = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(vendor_thing_id != NULL);
    M_KII_ASSERT(thing_password != NULL);
    M_KII_ASSERT(out_thing != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "oauth2", "token", NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, NULL,
                                                   "application/json");
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare request data */
    ret = prv_prepare_authenticate_thing_request_data(vendor_thing_id,
                                                      thing_password,
                                                      &reqStr);
    if (ret != KIIE_OK) {
        goto ON_EXIT;
    }
    if (kii_http_execute("POST", reqUrl, headers, reqStr, &respCode, NULL,
                         &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT;
    }

    ret = prv_parse_authenticate_thing_response(respCode, respData, out_thing,
                                            out_access_token, &err);
ON_EXIT:
    json_decref(headers);
    M_KII_FREE_NULLIFY(reqStr);
    M_KII_FREE_NULLIFY(respData);
    M_KII_FREE_NULLIFY(reqUrl);
    
    prv_kii_set_last_error(app, ret, &err);
    
    return ret;
}

kii_bucket_t kii_init_thing_bucket(const kii_thing_t thing,
                                   const kii_char_t* bucket_name)
{
    prv_kii_bucket_t* retval = NULL;
    kii_char_t* thing_id = NULL;
    kii_char_t* bucket_name_str = NULL;

    M_KII_ASSERT(thing != NULL);
    M_KII_ASSERT(thing->kii_thing_id != NULL);
    M_KII_ASSERT(kii_strlen(thing->kii_thing_id) > 0);
    M_KII_ASSERT(bucket_name != NULL);
    M_KII_ASSERT(kii_strlen(bucket_name) > 0);
    
    retval = kii_malloc(sizeof(prv_kii_bucket_t));
    thing_id = kii_strdup(thing->kii_thing_id);
    bucket_name_str =  kii_strdup(bucket_name);

    if (retval == NULL || thing_id == NULL ||
            bucket_name_str == NULL) {
        M_KII_FREE_NULLIFY(retval);
        M_KII_FREE_NULLIFY(thing_id);
        M_KII_FREE_NULLIFY(bucket_name_str);
        return NULL;
    }
    retval->kii_thing_id = thing_id;
    retval->bucket_name = bucket_name_str;
    return retval;
}

static kii_error_code_t prv_parse_create_new_object_response(
        kii_int_t respCode,
        const kii_char_t* respData,
        const json_t* respHdr,
        kii_char_t** out_object_id,
        kii_char_t** out_etag,
        kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* respJson = NULL;

    if (respCode < 200 || respCode >= 300) {
        ret = prv_parse_response_error_code(respCode, respData, err);
        goto ON_EXIT;
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag =
            json_string_value(json_object_get(respHdr, "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
            goto ON_EXIT;
        }
    }

    /* Check response data */
    if (out_object_id != NULL) {
        json_error_t jErr;
        respJson = json_loads(respData, 0, &jErr);
        if (respJson == NULL) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        } else  {
            const kii_char_t* objectID = json_string_value(json_object_get(
                    respJson, "objectID"));
            if (objectID != NULL) {
                *out_object_id = kii_strdup(objectID);
                ret = (*out_object_id != NULL) ? KIIE_OK : KIIE_LOWMEMORY;
                goto ON_EXIT;
            } else {
                prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
                ret = KIIE_FAIL;
                goto ON_EXIT;
            }
        }
    } else {
        ret = KIIE_OK;
        goto ON_EXIT;
    }

ON_EXIT:
    json_decref(respJson);
    return ret;
}

kii_error_code_t kii_create_new_object(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_bucket_t bucket,
                                       const json_t* contents,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag)
{
    kii_char_t *reqUrl = NULL;
    json_t* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(kii_strlen(bucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(bucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(contents != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            bucket->kii_thing_id, "buckets", bucket->bucket_name,
            "objects", NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, access_token,
            "application/json");
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqStr = json_dumps(contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("POST", reqUrl, headers, reqStr, &respCode, &respHdr,
                &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

    ret = prv_parse_create_new_object_response(respCode, respData, respHdr,
            out_object_id, out_etag, &err);

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    json_decref(headers);
    M_KII_FREE_NULLIFY(reqStr);
    json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

static kii_error_code_t prv_parse_create_new_object_with_id_response(
        kii_int_t respCode,
        const kii_char_t* respData,
        const json_t* respHdr,
        kii_char_t** out_etag,
        kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;

    if (respCode < 200 || respCode >= 300) {
        return prv_parse_response_error_code(respCode, respData, err);
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr,
                "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
            } else {
                ret = KIIE_OK;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    } else {
        ret = KIIE_OK;
    }

    return ret;
}

kii_error_code_t kii_create_new_object_with_id(kii_app_t app,
                                               const kii_char_t* access_token,
                                               const kii_bucket_t bucket,
                                               const kii_char_t* object_id,
                                               const json_t* contents,
                                               kii_char_t** out_etag)
{
    kii_char_t* reqUrl = NULL;
    json_t* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(kii_strlen(bucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(bucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(contents != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            bucket->kii_thing_id, "buckets", bucket->bucket_name,
            "objects", object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, access_token,
            "application/json");
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        if (json_object_set_new(headers, "if-none-match", json_string("*"))
                != 0) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
    }

    reqStr = json_dumps(contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("PUT", reqUrl, headers, reqStr, &respCode, &respHdr,
                &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

    ret = prv_parse_create_new_object_with_id_response(respCode, respData,
            respHdr, out_etag, &err);

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    json_decref(headers);
    M_KII_FREE_NULLIFY(reqStr);
    json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

static kii_error_code_t prv_parse_patch_object_response(
        kii_int_t respCode,
        kii_char_t* respData,
        const json_t* respHdr,
        kii_char_t** out_etag,
        kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;

    if (respCode < 200 || respCode >= 300) {
        return prv_parse_response_error_code(respCode, respData, err);
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr,
                "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
            } else {
                ret = KIIE_OK;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    }

    return ret;
}

kii_error_code_t kii_patch_object(kii_app_t app,
                                  const kii_char_t* access_token,
                                  const kii_bucket_t bucket,
                                  const kii_char_t* object_id,
                                  const json_t* patch,
                                  const kii_char_t* opt_etag,
                                  kii_char_t** out_etag)
{
    kii_char_t *reqUrl = NULL;
    json_t* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(kii_strlen(bucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(bucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(patch != NULL);
    M_KII_ASSERT(out_etag != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            bucket->kii_thing_id, "buckets", bucket->bucket_name, "objects",
            object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, access_token,
            "application/json");
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        kii_int_t json_set_result = opt_etag == NULL ?
            json_object_set_new(headers, "if-none-match", json_string("*")) :
            json_object_set_new(headers, "if-match", json_string(opt_etag));
        if (json_set_result != 0) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
    }

    reqStr = json_dumps(patch, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("PATCH", reqUrl, headers, reqStr, &respCode, &respHdr,
                &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

    ret = prv_parse_patch_object_response(respCode, respData, respHdr, out_etag,
            &err);

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    json_decref(headers);
    M_KII_FREE_NULLIFY(reqStr);
    json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);
    json_decref(respJson);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

static kii_error_code_t prv_parse_replace_object_response(
        kii_int_t respCode,
        kii_char_t* respData,
        const json_t* respHdr,
        kii_char_t** out_etag,
        kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;

    if (respCode < 200 || respCode >= 300) {
        return prv_parse_response_error_code(respCode, respData, err);
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr,
                        "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
            } else {
                ret = KIIE_OK;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    } else {
        ret = KIIE_OK;
    }

    return ret;
}

kii_error_code_t kii_replace_object(kii_app_t app,
                                    const kii_char_t* access_token,
                                    const kii_bucket_t bucket,
                                    const kii_char_t* object_id,
                                    const json_t* replace_contents,
                                    const kii_char_t* opt_etag,
                                    kii_char_t** out_etag)
{
    kii_char_t* reqUrl = NULL;
    json_t* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(kii_strlen(bucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(bucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(replace_contents != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            bucket->kii_thing_id, "buckets", bucket->bucket_name,
            "objects", object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, access_token,
            "application/json");
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    if (opt_etag != NULL) {
        if (json_object_set_new(headers, "if-match", json_string(opt_etag))
                != 0) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
    }
    reqStr = json_dumps(replace_contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("PUT", reqUrl, headers, reqStr, &respCode, &respHdr,
                &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

    ret = prv_parse_replace_object_response(respCode, respData, respHdr,
            out_etag, &err);
ON_EXIT:
    kii_dispose_kii_char(reqUrl);
    json_decref(headers);
    kii_dispose_kii_char(reqStr);
    json_decref(respHdr);
    kii_dispose_kii_char(respData);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

static kii_error_code_t prv_parse_get_object_response(
        kii_int_t respCode,
        const json_t* respHdr,
        const kii_char_t* respData,
        json_t** out_contents,
        kii_char_t** out_etag,
        kii_error_t* err)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_error_t jErr;

    M_KII_ASSERT(out_etag != NULL);

    if (respCode < 200 || respCode >= 300) {
      ret = prv_parse_response_error_code(respCode, respData, err);
      goto ON_EXIT;
    }

    *out_contents = json_loads(respData, 0, &jErr);
    if (*out_contents == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Check response header */
    if (respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr,
                    "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
            } else {
                ret = KIIE_OK;
            }
        } else {
            prv_kii_set_info_in_error(err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    } else {
        ret = KIIE_OK;
    }

ON_EXIT:
    return ret;
}

kii_error_code_t kii_get_object(kii_app_t app,
                                const kii_char_t* access_token,
                                const kii_bucket_t bucket,
                                const kii_char_t* object_id,
                                json_t** out_contents,
                                kii_char_t** out_etag)
{
    kii_char_t *reqUrl = NULL;
    json_t* headers = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respHdr = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(out_contents != NULL);
    M_KII_ASSERT(out_etag != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            bucket->kii_thing_id, "buckets", bucket->bucket_name, "objects",
            object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, access_token, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("GET", reqUrl, headers, NULL, &respCode, &respHdr,
                &respData) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

    ret = prv_parse_get_object_response(respCode, respHdr, respData, out_contents,
            out_etag, &err);

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    json_decref(headers);
    M_KII_FREE_NULLIFY(respData);
    json_decref(respHdr);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

kii_error_code_t kii_delete_object(kii_app_t app,
                                   const kii_char_t* access_token,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id)
{
    kii_char_t *reqUrl = NULL;
    json_t* headers = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(object_id != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(app->site_url, "apps", app->app_id, "things",
            bucket->kii_thing_id, "buckets", bucket->bucket_name, "objects",
            object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    headers = prv_create_common_header_json_object(app, access_token, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("DELETE", reqUrl, headers, NULL, &respCode, NULL,
                &respData) == KII_TRUE) {
        if (respCode < 200 || respCode >= 300) {
            ret = prv_parse_response_error_code(respCode, respData, &err);
            goto ON_EXIT;
        } else {
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    json_decref(headers);
    M_KII_FREE_NULLIFY(respData);

    prv_kii_set_last_error(app, ret, &err);

    return ret;
}

kii_error_code_t kii_subscribe_bucket(kii_app_t app,
                                      const kii_char_t* access_token,
                                      const kii_bucket_t bucket)
{
    const kii_char_t* thingId = bucket->kii_thing_id;
    const kii_char_t* bucketName = bucket->bucket_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t respStatus = 0;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "buckets",
                        bucketName,
                        "filters/all/push/subscriptions/things",
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("POST", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || (respStatus >= 300 && respStatus != 409)) {
            ret = prv_parse_response_error_code(respStatus, respBodyStr,
                    &error);
            goto ON_EXIT;
        } else {
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    json_decref(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);

    prv_kii_set_last_error(app, ret, &error);
    return ret;
}

kii_error_code_t kii_unsubscribe_bucket(kii_app_t app,
                                        const kii_char_t* access_token,
                                        const kii_bucket_t bucket)
{
    const kii_char_t* thingId = bucket->kii_thing_id;
    const kii_char_t* bucketName = bucket->bucket_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t respStatus = 0;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "buckets",
                        bucketName,
                        "filters/all/push/subscriptions/things",
                        thingId,
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("DELETE", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || respStatus >= 300) {
            ret = prv_parse_response_error_code(respStatus, respBodyStr,
                    &error);
            goto ON_EXIT;
        } else {
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    json_decref(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);

    prv_kii_set_last_error(app, ret, &error);
    return ret;
}

kii_error_code_t kii_is_bucket_subscribed(kii_app_t app,
                                          const kii_char_t* access_token,
                                          const kii_bucket_t bucket,
                                          kii_bool_t* out_is_subscribed)
{
    const kii_char_t* thingId = bucket->kii_thing_id;
    const kii_char_t* bucketName = bucket->bucket_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t respStatus = 0;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "buckets",
                        bucketName,
                        "filters/all/push/subscriptions/things",
                        thingId,
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("HEAD", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || respStatus >= 300) {
            if (respStatus == 404) {
                *out_is_subscribed = KII_FALSE;
                ret = KIIE_OK;
            } else {
                ret = prv_parse_response_error_code(respStatus, respBodyStr,
                        &error);
            }
            goto ON_EXIT;
        } else {
            *out_is_subscribed = KII_TRUE;
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    json_decref(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);

    prv_kii_set_last_error(app, ret, &error);
    return ret;
}

kii_topic_t kii_init_thing_topic(const kii_thing_t thing,
                                 const kii_char_t* topic_name)
{
    prv_kii_topic_t* topic = NULL;
    kii_char_t* tempThingId = NULL;
    kii_char_t* tempTopicName = NULL;
    

    M_KII_ASSERT(thing != NULL);
    M_KII_ASSERT(thing->kii_thing_id != NULL);
    M_KII_ASSERT(kii_strlen(thing->kii_thing_id) > 0);
    M_KII_ASSERT(topic_name != NULL);
    M_KII_ASSERT(kii_strlen(topic_name) > 0);

    topic = kii_malloc(sizeof(prv_kii_topic_t));
    if (topic == NULL) {
        return NULL;
    }

    tempThingId = kii_strdup(thing->kii_thing_id);
    if (tempThingId == NULL) {
        M_KII_FREE_NULLIFY(topic);
        return NULL;
    }

    tempTopicName= kii_strdup(topic_name);
    if (tempTopicName == NULL) {
        M_KII_FREE_NULLIFY(tempThingId);
        M_KII_FREE_NULLIFY(topic);
        return NULL;
    }

    topic->kii_thing_id = tempThingId;
    topic->topic_name = tempTopicName;
    return topic;
}

kii_error_code_t kii_create_topic(kii_app_t app,
                                  const kii_char_t* access_token,
                                  const kii_topic_t topic)
{
    const kii_char_t* thingId = topic->kii_thing_id;
    const kii_char_t* topicName = topic->topic_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t respStatus = 0;
    kii_char_t* respBodyStr = NULL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "topics",
                        topicName,
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("PUT", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || (respStatus >= 300 && respStatus != 409)) {
            ret = prv_parse_response_error_code(respStatus, respBodyStr,
                    &error);
            goto ON_EXIT;
        } else {
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    json_decref(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}

kii_error_code_t kii_subscribe_topic(kii_app_t app,
                                     const kii_char_t* access_token,
                                     const kii_topic_t topic)
{
    const kii_char_t* thingId = topic->kii_thing_id;
    const kii_char_t* topicName = topic->topic_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t respStatus = 0;
    kii_char_t* respBodyStr = NULL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "topics",
                        topicName,
                        "push",
                        "subscriptions",
                        "things",
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("POST", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || (respStatus >= 300 && respStatus != 409)) {
            ret = prv_parse_response_error_code(respStatus, respBodyStr,
                    &error);
            goto ON_EXIT;
        } else {
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    json_decref(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}

kii_error_code_t kii_unsubscribe_topic(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_topic_t topic)
{
    const kii_char_t* thingId = topic->kii_thing_id;
    const kii_char_t* topicName = topic->topic_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t respStatus = 0;
    kii_char_t* respBodyStr = NULL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "topics",
                        topicName,
                        "push",
                        "subscriptions",
                        "things",
                        thingId,
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("DELETE", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || respStatus >= 300) {
            ret = prv_parse_response_error_code(respStatus, respBodyStr,
                    &error);
            goto ON_EXIT;
        } else {
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    json_decref(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}

kii_error_code_t kii_is_topic_subscribed(kii_app_t app,
                                         const kii_char_t* access_token,
                                         const kii_topic_t topic,
                                         kii_bool_t* out_is_subscribed)
{
    const kii_char_t* thingId = topic->kii_thing_id;
    const kii_char_t* topicName = topic->topic_name;
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_int_t respStatus = 0;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "things",
                        thingId,
                        "topics",
                        topicName,
                        "push",
                        "subscriptions",
                        "things",
                        thingId,
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("HEAD", url, reqHeaders, NULL, &respStatus, NULL,
                &respBodyStr) == KII_TRUE) {
        if (respStatus < 200 || respStatus >= 300) {
            if (respStatus == 404) {
                *out_is_subscribed = KII_FALSE;
                ret = KIIE_OK;
            } else {
                ret = prv_parse_response_error_code(respStatus, respBodyStr,
                        &error);
            }
            goto ON_EXIT;
        } else {
            *out_is_subscribed = KII_TRUE;
            ret = KIIE_OK;
        }
    } else {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

ON_EXIT:
    kii_dispose_kii_char(url);
    json_decref(reqHeaders);
    kii_dispose_kii_char(respBodyStr);

    prv_kii_set_last_error(app, ret, &error);

    return ret;
}

static kii_error_code_t prv_prepare_install_thing_push_request_data(
        kii_bool_t development,
        kii_char_t** out_string)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* reqJson = NULL;
    kii_int_t json_set_result = 0;

    reqJson = json_object();
    if (reqJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    json_set_result = 0;
    json_set_result |= json_object_set_new(reqJson, "deviceType",
            json_string("MQTT"));
    json_set_result |= json_object_set_new(reqJson, "development",
            development == KII_FALSE ? json_false() : json_true());
    if (json_set_result != 0) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    *out_string = json_dumps(reqJson, 0);
    ret = (*out_string == NULL) ? KIIE_LOWMEMORY : KIIE_OK;

ON_EXIT:
    json_decref(reqJson);
    return ret;
}

static kii_error_code_t prv_parse_install_thing_push_response(
        kii_int_t respCode,
        const kii_char_t* respBodyStr,
        kii_char_t** out_installation_id,
        kii_error_t* error)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_t* respBodyJson = NULL;
    json_error_t jErr;

    if (respCode < 200 || respCode >= 300) {
        return prv_parse_response_error_code(respCode, respBodyStr, error);
    }

    /* Parse body */
    respBodyJson = json_loads(respBodyStr, 0, &jErr);
    if (respBodyJson == NULL) {
        ret = KIIE_LOWMEMORY;
    } else {
        json_t* installIDJson = json_object_get(respBodyJson, "installationID");
        if (installIDJson != NULL) {
            *out_installation_id = kii_strdup(json_string_value(installIDJson));
            ret = *out_installation_id != NULL ? KIIE_OK : KIIE_LOWMEMORY;
        } else {
            prv_kii_set_info_in_error(error, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    }

    json_decref(respBodyJson);
    return ret;
}

kii_error_code_t kii_install_thing_push(kii_app_t app,
                                        const kii_char_t* access_token,
                                        kii_bool_t development,
                                        kii_char_t** out_installation_id)
{
    kii_char_t* url = NULL;
    kii_char_t* reqBodyStr = NULL;
    json_t* reqHeaders = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    
    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(out_installation_id != NULL);

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare URL */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "installations",
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    /* Prepare body */
    ret = prv_prepare_install_thing_push_request_data(development, &reqBodyStr);
    if (ret != KIIE_OK) {
        goto ON_EXIT;
    }

    /* Prepare headers*/
    reqHeaders = prv_create_common_header_json_object(app, access_token,
            "application/vnd.kii.InstallationCreationRequest+json");
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("POST", url, reqHeaders, reqBodyStr, &respCode, NULL,
                &respBodyStr) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT; 
    }

    ret = prv_parse_install_thing_push_response(respCode, respBodyStr,
            out_installation_id, &error);

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(reqBodyStr);
    M_KII_FREE_NULLIFY(respBodyStr);
    json_decref(reqHeaders);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}


static kii_error_code_t prv_parse_endpoint(
        kii_int_t respCode,
        const kii_char_t* respBodyStr,
        kii_mqtt_endpoint_t** out_endpoint,
        kii_uint_t* out_retry_after_in_second,
        kii_error_t* error)
{
    kii_error_code_t ret = KIIE_FAIL;
    json_error_t jErr;
    json_t* respBodyJson = NULL;

    if ((respCode < 200 || respCode >= 300) && respCode != 503) {
        ret = prv_parse_response_error_code(respCode, respBodyStr, error);
        goto ON_EXIT;
    } else if (respCode == 503) {
        respBodyJson = json_loads(respBodyStr, 0, &jErr);
        if (respBodyJson == NULL) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        } else {
            int retryAfterInt = 0;
            json_t* retryAfterJson = json_object_get(respBodyJson,
                    "retryAfter");
            retryAfterInt = (int)json_integer_value(retryAfterJson);
            if (retryAfterInt > 0) {
                *out_retry_after_in_second = retryAfterInt;
            }
            ret = prv_parse_response_error_code(respCode, respBodyStr, error);
            goto ON_EXIT;
        }
    }

    respBodyJson = json_loads(respBodyStr, 0, &jErr);
    if (respBodyJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        const json_t* userNameJson = json_object_get(respBodyJson, "username");
        const json_t* passwordJson = json_object_get(respBodyJson, "password");
        const json_t* mqttTopicJson = json_object_get(respBodyJson,
                "mqttTopic");
        const json_t* hostJson = json_object_get(respBodyJson, "host");
        const json_t* mqttTtlJson = json_object_get(respBodyJson, "X-MQTT-TTL");
        const json_t* portTcpJson = json_object_get(respBodyJson, "portTCP");
        const json_t* portSslJson = json_object_get(respBodyJson, "portSSL");
        if (userNameJson == NULL || passwordJson == NULL ||
            mqttTopicJson == NULL || hostJson == NULL || mqttTtlJson == NULL ||
            portTcpJson == NULL || portSslJson == NULL) {
            prv_kii_set_info_in_error(error, 0, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
            goto ON_EXIT;
        }

        {
            kii_mqtt_endpoint_t* endpoint =
                kii_malloc(sizeof(kii_mqtt_endpoint_t));
            kii_char_t* username = kii_strdup(json_string_value(userNameJson));
            kii_char_t* password = kii_strdup(json_string_value(passwordJson));
            kii_char_t* topic = kii_strdup(json_string_value(mqttTopicJson));
            kii_char_t* host = kii_strdup(json_string_value(hostJson));
            kii_uint_t portTcpInt = (kii_uint_t)json_integer_value(portTcpJson);
            kii_uint_t portSslInt = (kii_uint_t)json_integer_value(portSslJson);
            kii_ulong_t ttl = (kii_ulong_t)json_integer_value(mqttTtlJson);

            if (endpoint == NULL ||
                    username == NULL ||
                    password == NULL ||
                    topic == NULL ||
                    host == NULL) {
                M_KII_FREE_NULLIFY(endpoint);
                M_KII_FREE_NULLIFY(username);
                M_KII_FREE_NULLIFY(password);
                M_KII_FREE_NULLIFY(topic);
                M_KII_FREE_NULLIFY(host);
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }

            endpoint->username = username;
            endpoint->password = password;
            endpoint->topic = topic;
            endpoint->host = host;
            endpoint->ttl = ttl;
            endpoint->port_tcp = portTcpInt;
            endpoint->port_ssl = portSslInt;
            *out_endpoint = endpoint;
        }

        ret = KIIE_OK;
    }

ON_EXIT:
    json_decref(respBodyJson);
    return ret;
}

kii_error_code_t kii_get_mqtt_endpoint(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_char_t* installation_id,
                                       kii_mqtt_endpoint_t** out_endpoint,
                                       kii_uint_t* out_retry_after_in_second)
{
    kii_char_t* url = NULL;
    json_t* reqHeaders = NULL;
    kii_int_t respCode = 0;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(out_endpoint != NULL);

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare URL */
    url = prv_build_url(app->site_url,
                        "apps",
                        app->app_id,
                        "installations",
                        installation_id,
                        "mqtt-endpoint",
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    M_KII_DEBUG(prv_log("mqtt endpoint url: %s", url));
    /* Prepare Headers */
    reqHeaders = prv_create_common_header_json_object(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    if (kii_http_execute("GET", url, reqHeaders, NULL, &respCode, NULL,
                &respBodyStr) == KII_FALSE) {
        ret = KIIE_ADAPTER;
        goto ON_EXIT;
    }

    /* Parse body */
    ret = prv_parse_endpoint(respCode, respBodyStr, out_endpoint,
            out_retry_after_in_second, &error);

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(respBodyStr);
    json_decref(reqHeaders);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}
