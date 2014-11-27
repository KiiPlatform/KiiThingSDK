/*
  kii_cloud.c
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#include "curl.h"
#include "kii_custom.h"
#include "kii_prv_utils.h"
#include "kii_prv_types.h"

kii_error_code_t kii_global_init(void)
{
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    return ((r == CURLE_OK) ? KIIE_OK : KIIE_FAIL);
}

void kii_global_cleanup(void)
{
    curl_global_cleanup();
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
    
    app->curl_easy = curl_easy_init();
    if (app->curl_easy == NULL) {
        M_KII_FREE_NULLIFY(app->app_id);
        M_KII_FREE_NULLIFY(app->app_key);
        M_KII_FREE_NULLIFY(app->site_url);
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
    curl_easy_cleanup(app->curl_easy);
    app->curl_easy = NULL;
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

static size_t callbackWrite(char* ptr,
                            size_t size,
                            size_t nmemb,
                            kii_char_t** respData)
{
    size_t dataLen = size * nmemb;
    if (dataLen == 0) {
        return 0;
    }
    if (*respData == NULL) { /* First time. */
        *respData = kii_malloc(dataLen+1);
        if (respData == NULL) {
            return 0;
        }
        kii_memcpy(*respData, ptr, dataLen);
        (*respData)[dataLen] = '\0';
    } else {
        size_t lastLen = kii_strlen(*respData);
        size_t newLen = lastLen + dataLen;
        kii_char_t* concat = kii_realloc(*respData, newLen + 1);
        if (concat == NULL) {
            return 0;
        }
        kii_strncat(concat, ptr, dataLen);
        *respData = concat;
    }
    return dataLen;
}

static size_t callback_header(
        char *buffer,
        size_t size,
        size_t nitems,
        void *userdata)
{
    const char ETAG[] = "etag";
    size_t len = size * nitems;
    size_t ret = len;
    kii_char_t* line = kii_malloc(len + 1);

    M_KII_ASSERT(userdata != NULL);

    if (line == NULL) {
        return 0;
    }

    {
        int i = 0;
        kii_memcpy(line, buffer, len);
        line[len] = '\0';
        M_KII_DEBUG(prv_log_no_LF("resp header: %s", line));
        /* Field name becomes upper case. */
        for (i = 0; line[i] != '\0' && line[i] != ':'; ++i) {
            line[i] = (char)kii_tolower(line[i]);
        }
    }

    /* check http header name. */
    if (kii_strncmp(line, ETAG, kii_strlen(ETAG)) == 0) {
        json_t** json = userdata;
        int i = 0;
        kii_char_t* value = line;

        /* change line feed code lasting end of this array to '\0'. */
        for (i = (int)len; i >= 0; --i) {
            if (line[i] == '\0') {
                continue;
            } else if (line[i] == '\r' || line[i] == '\n') {
                line[i] = '\0';
            } else {
                break;
            }
        }

        /* skip until ":". */
        while (*value != ':') {
            ++value;
        }
        /* skip ':' */
        ++value;
        /* skip spaces. */
        while (*value == ' ') {
            ++value;
        }

        if (*json == NULL) {
            *json = json_object();
            if (*json == NULL) {
                ret = 0;
                goto ON_EXIT;
            }
        }
        if (json_object_set_new(*json, ETAG, json_string(value)) != 0) {
            ret = 0;
            goto ON_EXIT;
        }
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(line);
    return ret;
}

typedef enum {
    POST,
    PUT,
    PATCH,
    DELETE,
    GET,
    HEAD
} prv_kii_req_method_t;

kii_error_code_t prv_execute_curl(CURL* curl,
                                  const kii_char_t* url,
                                  prv_kii_req_method_t method,
                                  const kii_char_t* request_body,
                                  struct curl_slist* request_headers,
                                  long* response_status_code,
                                  kii_char_t** response_body,
                                  json_t** response_headers,
                                  kii_error_t* error)
{
    CURLcode curlCode = CURLE_COULDNT_CONNECT; /* set error code as default. */

    M_KII_ASSERT(curl != NULL);
    M_KII_ASSERT(url != NULL);
    M_KII_ASSERT(request_headers != NULL);
    M_KII_ASSERT(response_status_code != NULL);
    M_KII_ASSERT(error != NULL);

    M_KII_DEBUG(prv_log("request url: %s", url));
    M_KII_DEBUG(prv_log("request method: %d", method));
    M_KII_DEBUG(prv_log("request body: %s", request_body));

    /* reset previous session setting. */
    curl_easy_reset(curl);

    switch (method) {
        case POST:
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
            if (request_body == NULL || kii_strlen(request_body) == 0) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
            }
            break;
        case PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
            if (request_body == NULL || kii_strlen(request_body) == 0) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
            }
            break;
        case PATCH:
            request_headers = curl_slist_append(request_headers,
                    "X-HTTP-METHOD-OVERRIDE: PATCH");
            if (request_headers == NULL) {
                return KIIE_LOWMEMORY;
            }
            if (request_body != NULL) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
            }
            break;
        case DELETE:
            M_KII_ASSERT(request_body == NULL);
            curl_easy_setopt(curl,CURLOPT_CUSTOMREQUEST,"DELETE");
            break;
        case GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            if (request_body != NULL) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
            }
            break;
        case HEAD:
            M_KII_ASSERT(request_body == NULL);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
            break;
        default:
            M_KII_ASSERT(0); /* programing error */
            return KIIE_FAIL;
    }

    M_KII_DEBUG(prv_log_req_heder(request_headers));

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_body);
    if (response_headers != NULL) {
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, callback_header);
        *response_headers = NULL;
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, response_headers);
    }

    curlCode = curl_easy_perform(curl);
    switch (curlCode) {
        case CURLE_OK:
            M_KII_DEBUG(prv_log("response: %s", *response_body));
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                    response_status_code);
            if ((200 <= *response_status_code) &&
                    (*response_status_code < 300)) {
                return KIIE_OK;
            } else {
                const kii_char_t* error_code = NULL;
                json_t* errJson = NULL;
                json_t* errorCodeJson = NULL;
                if (*response_body != NULL) {
                    json_error_t jErr;
                    errJson = json_loads(*response_body, 0, &jErr);
                    if (errJson == NULL) {
                        return KIIE_LOWMEMORY;
                    }
                }
                errorCodeJson = json_object_get(errJson, "errorCode");
                if (errorCodeJson != NULL) {
                    error_code = json_string_value(errorCodeJson);
                } else {
                    error_code = *response_body;
                }
                prv_kii_set_info_in_error(error, (int)(*response_status_code),
                        error_code);
                json_decref(errJson);
                return KIIE_FAIL;
            }
        case CURLE_WRITE_ERROR:
            return KIIE_RESPWRITE;
        default:
            prv_kii_set_info_in_error(error, 0, KII_ECODE_CONNECTION);
            return KIIE_FAIL;
    }
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

kii_error_code_t kii_register_thing(kii_app_t app,
                                    const kii_char_t* vendor_thing_id,
                                    const kii_char_t* thing_password,
                                    const kii_char_t* opt_thing_type,
                                    const json_t* user_data,
                                    kii_thing_t* out_thing,
                                    kii_char_t** out_access_token)
{
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    json_t* reqJson = NULL;
    kii_char_t* reqStr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;
    kii_int_t json_set_result = 0;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, NULL,
            "application/vnd.kii.ThingRegistrationAndAuthorizationRequest+json");
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    /* prepare request data */
    reqJson = (user_data == NULL) ? json_object() :
            json_deep_copy(user_data);
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

    reqStr = json_dumps(reqJson, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(app->curl_easy, reqUrl, POST,
            reqStr, headers, &respCode, &respData, NULL, &err);
    if (exeCurlRet != KIIE_OK) {
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Check response code */
    {
        json_error_t jErr;
        respJson = json_loads(respData, 0, &jErr);

        if (respJson == NULL) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        } else {
            const kii_char_t* accessToken =
                json_string_value(json_object_get(respJson, "_accessToken"));
            const kii_char_t* thingId =
                json_string_value(json_object_get(respJson, "_thingID"));
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
                prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
                ret = KIIE_FAIL;
                goto ON_EXIT;
            }
        }
        goto ON_EXIT;
    }
    
ON_EXIT:
    curl_slist_free_all(headers);
    json_decref(reqJson);
    M_KII_FREE_NULLIFY(reqStr);
    M_KII_FREE_NULLIFY(respData);
    M_KII_FREE_NULLIFY(reqUrl);
    json_decref(respJson);

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

kii_error_code_t kii_create_new_object(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_bucket_t bucket,
                                       const json_t* contents,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag)
{
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, access_token,
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

    exeCurlRet = prv_execute_curl(app->curl_easy, reqUrl, POST,
            reqStr, headers, &respCode, &respData, &respHdr, &err);
    if (exeCurlRet != KIIE_OK) {
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr, "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }
        } else {
            prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
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
            const kii_char_t* objectID = json_string_value(json_object_get(respJson,
                    "objectID"));
            if (objectID != NULL) {
                *out_object_id = kii_strdup(objectID);
                ret = (*out_object_id != NULL) ? KIIE_OK : KIIE_LOWMEMORY;
                goto ON_EXIT;
            } else {
                prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
                ret = KIIE_FAIL;
                goto ON_EXIT;
            }
        }
    } else {
        ret = KIIE_OK;
        goto ON_EXIT;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    curl_slist_free_all(headers);
    M_KII_FREE_NULLIFY(reqStr);
    json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);
    json_decref(respJson);

    prv_kii_set_last_error(app, ret, &err);

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
    struct curl_slist* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, access_token,
            "application/json");
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        struct curl_slist* tmp = curl_slist_append(headers, "if-none-match: *");
        if (tmp == NULL) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
        headers = tmp;
    }

    reqStr = json_dumps(contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(app->curl_easy, reqUrl, PUT,
            reqStr, headers, &respCode, &respData, &respHdr, &err);
    if (exeCurlRet != KIIE_OK) {
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr,
                "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }
            ret = KIIE_OK;
        } else {
            prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    } else {
        ret = KIIE_OK;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    curl_slist_free_all(headers);
    M_KII_FREE_NULLIFY(reqStr);
    json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);

    prv_kii_set_last_error(app, ret, &err);

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
    struct curl_slist* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, access_token,
            "application/json");
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        struct curl_slist* tmp = opt_etag == NULL ?
            curl_slist_append(headers, "if-none-match: *") :
            prv_curl_slist_append_key_and_value(headers, "if-match", opt_etag);
        if (tmp == NULL) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
        headers = tmp;
    }

    reqStr = json_dumps(patch, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy, reqUrl, PATCH,
            reqStr, headers, &respCode, &respData, &respHdr, &err);
    if (ret != KIIE_OK) {
        goto ON_EXIT;
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr, "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }
        } else {
            prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
            goto ON_EXIT;
        }
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    curl_slist_free_all(headers);
    M_KII_FREE_NULLIFY(reqStr);
    json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);
    json_decref(respJson);

    prv_kii_set_last_error(app, ret, &err);

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
    struct curl_slist* headers = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, access_token,
            "application/json");
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    if (opt_etag != NULL) {
        struct curl_slist* tmp =
            prv_curl_slist_append_key_and_value(headers, "if-match", opt_etag);
        if (tmp == NULL) {
            ret = KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
        headers = tmp;
    }

    reqStr = json_dumps(replace_contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(app->curl_easy, reqUrl, PUT,
            reqStr, headers, &respCode, &respData, &respHdr, &err);
    if (exeCurlRet != KIIE_OK) {
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Check response header */
    if (out_etag != NULL && respHdr != NULL) {
        const char* etag = json_string_value(json_object_get(respHdr,
                "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }
            ret = KIIE_OK;
        } else {
            prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
        }
    } else {
        ret = KIIE_OK;
    }

ON_EXIT:
    kii_dispose_kii_char(reqUrl);
    curl_slist_free_all(headers);
    kii_dispose_kii_char(reqStr);
    json_decref(respHdr);
    kii_dispose_kii_char(respData);

    prv_kii_set_last_error(app, ret, &err);

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
    struct curl_slist* headers = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respHdr = NULL;
    json_error_t jErr;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, access_token, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy, reqUrl, GET,
            NULL, headers, &respCode, &respData, &respHdr, &err);
    if (ret != KIIE_OK) {
        goto ON_EXIT;
    }

    *out_contents = json_loads(respData, 0, &jErr);
    if (*out_contents == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Check response header */
    if (respHdr != NULL) {
        const kii_char_t* etag = json_string_value(json_object_get(respHdr, "etag"));
        if (etag != NULL) {
            *out_etag = kii_strdup(etag);
            if (*out_etag == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            }
        } else {
            prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
            goto ON_EXIT;
        }
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    curl_slist_free_all(headers);
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
    struct curl_slist* headers = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(app->app_id)>0);
    M_KII_ASSERT(kii_strlen(app->app_key)>0);
    M_KII_ASSERT(kii_strlen(app->site_url)>0);
    M_KII_ASSERT(app->curl_easy != NULL);
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
    headers = prv_common_request_headers(app, access_token, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy, reqUrl, DELETE,
            NULL, headers, &respCode, &respData, NULL, &err);

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    curl_slist_free_all(headers);
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
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;

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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                           url,
                           POST,
                           NULL,
                           reqHeaders,
                           &respStatus,
                           &respBodyStr,
                           NULL,
                           &error);
    if (respStatus == 409) {
        /* bucket is already subscribed. */
        ret = KIIE_OK;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    curl_slist_free_all(reqHeaders);
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
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;

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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                           url,
                           DELETE,
                           NULL,
                           reqHeaders,
                           &respStatus,
                           &respBodyStr,
                           NULL,
                           &error);

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    curl_slist_free_all(reqHeaders);
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
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;

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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                           url,
                           HEAD,
                           NULL,
                           reqHeaders,
                           &respStatus,
                           &respBodyStr,
                           NULL,
                           &error);
    switch (ret) {
        case KIIE_OK:
            *out_is_subscribed = KII_TRUE;
            break;
        case KIIE_FAIL:
            if (error.status_code == 404) {
                *out_is_subscribed = KII_FALSE;
                ret = KIIE_OK;
                kii_memset(&error, 0, sizeof(kii_error_t));
            }
            break;
        default:
            /* nothing to do. */
            break;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    curl_slist_free_all(reqHeaders);
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
    struct curl_slist* reqHeaders = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;
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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                     url,
                     PUT,
                     NULL,
                     reqHeaders,
                     &respStatus,
                     &respBodyStr,
                     NULL,
                     &error);
    if (respStatus == 409) {
        /* topic already exists. */
        ret = KIIE_OK;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    curl_slist_free_all(reqHeaders);
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
    struct curl_slist* reqHeaders = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;
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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                     url,
                     POST,
                     NULL,
                     reqHeaders,
                     &respStatus,
                     &respBodyStr,
                     NULL,
                     &error);
    if (respStatus == 409) {
        /* topic is already subscribed. */
        ret = KIIE_OK;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    curl_slist_free_all(reqHeaders);
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
    struct curl_slist* reqHeaders = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;
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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                           url,
                           DELETE,
                           NULL,
                           reqHeaders,
                           &respStatus,
                           &respBodyStr,
                           NULL,
                           &error);

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    curl_slist_free_all(reqHeaders);
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
    struct curl_slist* reqHeaders = NULL;
    long respStatus = 0;
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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy,
                           url,
                           HEAD,
                           NULL,
                           reqHeaders,
                           &respStatus,
                           &respBodyStr,
                           NULL,
                           &error);
    switch (ret) {
        case KIIE_OK:
            *out_is_subscribed = KII_TRUE;
            break;
        case KIIE_FAIL:
            if (error.status_code == 404) {
                *out_is_subscribed = KII_FALSE;
                ret = KIIE_OK;
                kii_memset(&error, 0, sizeof(kii_error_t));
            }
            break;
        default:
            /* nothing to do */
            break;
    }

ON_EXIT:
    kii_dispose_kii_char(url);
    curl_slist_free_all(reqHeaders);
    kii_dispose_kii_char(respBodyStr);

    prv_kii_set_last_error(app, ret, &error);

    return ret;
}

kii_error_code_t kii_install_thing_push(kii_app_t app,
                                        const kii_char_t* access_token,
                                        kii_bool_t development,
                                        kii_char_t** out_installation_id)
{
    kii_char_t* url = NULL;
    json_t* reqBodyJson = NULL;
    kii_char_t* reqBodyStr = NULL;
    struct curl_slist* reqHeaders = NULL;
    long respCode = 0;
    kii_char_t* respBodyStr = NULL;
    json_t* respBodyJson = NULL;
    kii_error_t error;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;
    json_error_t jErr;
    kii_int_t json_set_result = 0;
    
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
    reqBodyJson = json_object();
    if (reqBodyJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    json_set_result = 0;
    json_set_result |= json_object_set_new(reqBodyJson, "deviceType",
            json_string("MQTT"));
    json_set_result |= json_object_set_new(reqBodyJson, "development",
            json_boolean(development));
    if (json_set_result != 0) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    reqBodyStr = json_dumps(reqBodyJson, 0);
    if (reqBodyStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* Prepare headers*/
    reqHeaders = prv_common_request_headers(app, access_token,
            "application/vnd.kii.InstallationCreationRequest+json");
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(app->curl_easy,
                                  url,
                                  POST,
                                  reqBodyStr,
                                  reqHeaders,
                                  &respCode,
                                  &respBodyStr,
                                  NULL,
                                  &error);
    if (exeCurlRet != KIIE_OK) {
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Parse body */
    respBodyJson = json_loads(respBodyStr, 0, &jErr);
    if (respBodyJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        json_t* installIDJson = NULL;
        installIDJson = json_object_get(respBodyJson, "installationID");
        if (installIDJson != NULL) {
            *out_installation_id = kii_strdup(json_string_value(installIDJson));
            ret = *out_installation_id != NULL ? KIIE_OK : KIIE_LOWMEMORY;
            goto ON_EXIT;
        } else {
            prv_kii_set_info_in_error(&error, (int)respCode, KII_ECODE_PARSE);
            ret = KIIE_FAIL;
            goto ON_EXIT;
        }
    }


ON_EXIT:
    json_decref(respBodyJson);
    M_KII_FREE_NULLIFY(url);
    json_decref(reqBodyJson);
    M_KII_FREE_NULLIFY(reqBodyStr);
    M_KII_FREE_NULLIFY(respBodyStr);
    curl_slist_free_all(reqHeaders);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}

kii_error_code_t kii_get_mqtt_endpoint(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_char_t* installation_id,
                                       kii_mqtt_endpoint_t** out_endpoint,
                                       kii_uint_t* out_retry_after_in_second)
{
    kii_char_t* url = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    long respCode = 0;
    kii_char_t* respBodyStr = NULL;
    json_t* respBodyJson = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    json_error_t jErr;
    json_t* userNameJson = NULL;
    json_t* passwordJson = NULL;
    json_t* mqttTopicJson = NULL;
    json_t* hostJson = NULL;
    json_t* mqttTtlJson = NULL;
    json_t* portTcpJson = NULL;
    json_t* portSslJson = NULL;

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
    reqHeaders = prv_common_request_headers(app, access_token, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(app->curl_easy,
                                  url,
                                  GET,
                                  NULL,
                                  reqHeaders,
                                  &respCode,
                                  &respBodyStr,
                                  NULL,
                                  &error);
    if (exeCurlRet != KIIE_OK) {
        if (error.status_code == 503) {
            json_t* retryAfterJson = NULL;
            respBodyJson = json_loads(respBodyStr, 0, &jErr);
            if (respBodyJson == NULL) {
                ret = KIIE_LOWMEMORY;
                goto ON_EXIT;
            } else {
                int retryAfterInt = 0;
                retryAfterJson = json_object_get(respBodyJson, "retryAfter");
                retryAfterInt = (int)json_integer_value(retryAfterJson);
                if (retryAfterInt) {
                    *out_retry_after_in_second = retryAfterInt;
                }
            }
        }
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Parse body */
    respBodyJson = json_loads(respBodyStr, 0, &jErr);
    if (respBodyJson == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    } else {
        userNameJson = json_object_get(respBodyJson, "username");
        passwordJson = json_object_get(respBodyJson, "password");
        mqttTopicJson = json_object_get(respBodyJson, "mqttTopic");
        hostJson = json_object_get(respBodyJson, "host");
        mqttTtlJson = json_object_get(respBodyJson, "X-MQTT-TTL");
        portTcpJson = json_object_get(respBodyJson, "portTCP");
        portSslJson = json_object_get(respBodyJson, "portSSL");
        if (userNameJson == NULL || passwordJson == NULL ||
            mqttTopicJson == NULL || hostJson == NULL || mqttTtlJson == NULL ||
            portTcpJson == NULL || portSslJson == NULL) {
            prv_kii_set_info_in_error(&error, 0, KII_ECODE_PARSE);
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
        goto ON_EXIT;
    }

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(respBodyStr);
    curl_slist_free_all(reqHeaders);
    json_decref(respBodyJson);
    prv_kii_set_last_error(app, ret, &error);

    return ret;
}
