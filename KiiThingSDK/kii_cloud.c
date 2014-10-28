/*
  kii_cloud.c
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#include "curl.h"
#include "kii_cloud.h"
#include "kii_logger.h"
#include "kii_libc.h"
#include "kii_utils.h"

kii_error_code_t kii_global_init(void)
{
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    return ((r == CURLE_OK) ? KIIE_OK : KIIE_FAIL);
}

void kii_global_cleanup(void)
{
    curl_global_cleanup();
}

typedef struct prv_kii_app_t {
    char* app_id;
    char* app_key;
    char* site_url;
    CURL* curl_easy;
    kii_error_t* last_error;
} prv_kii_app_t;

typedef struct prv_kii_bucket_t {
    char* vendor_thing_id;
    char* bucket_name;
} prv_kii_bucket_t;

typedef struct prv_kii_topic_t {
    char* vendor_thing_id;
    char* topic_name;
} prv_kii_topic_t;

typedef struct prv_kii_http_put_data {
    const char* request_body;
    size_t position;
    size_t length;
} prv_kii_http_put_data;

kii_app_t kii_init_app(const char* app_id,
                       const char* app_key,
                       const char* site_url)
{
    prv_kii_app_t* app = malloc(sizeof(prv_kii_app_t));
    if (app == NULL) {
        return app;
    }
    app->last_error = NULL;
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
    return ((prv_kii_app_t *)app)->last_error;
}

void prv_kii_set_error(prv_kii_app_t* app, kii_error_t* new_error)
{
    M_KII_FREE_NULLIFY(app->last_error);
    app->last_error = new_error;
}

static kii_error_t* prv_construct_kii_error(
        int status_code,
        const char* error_code)
{
    kii_error_t* retval = kii_malloc(sizeof(kii_error_t));
    retval->status_code = status_code;
    retval->error_code = kii_strdup(error_code == NULL ? "" : error_code);
    return retval;
}

void prv_kii_dispose_kii_error(kii_error_t* error)
{
    if (error != NULL) {
        M_KII_FREE_NULLIFY(error->error_code);
        M_KII_FREE_NULLIFY(error);
    }
}

void kii_dispose_app(kii_app_t app)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    M_KII_FREE_NULLIFY(pApp->app_id);
    M_KII_FREE_NULLIFY(pApp->app_key);
    M_KII_FREE_NULLIFY(pApp->site_url);
    prv_kii_dispose_kii_error(pApp->last_error);
    curl_easy_cleanup(pApp->curl_easy);
    pApp->curl_easy = NULL;
    M_KII_FREE_NULLIFY(app);
}

void kii_dispose_bucket(kii_bucket_t bucket)
{
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*) bucket;
    M_KII_FREE_NULLIFY(pBucket->bucket_name);
    M_KII_FREE_NULLIFY(pBucket->vendor_thing_id);
    M_KII_FREE_NULLIFY(bucket);
}

void kii_dispose_topic(kii_topic_t topic)
{
    prv_kii_topic_t* pTopic = (prv_kii_topic_t*) topic;
    M_KII_FREE_NULLIFY(pTopic->topic_name);
    M_KII_FREE_NULLIFY(pTopic->vendor_thing_id);
    M_KII_FREE_NULLIFY(topic);
}

void kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t* endpoint)
{
    M_KII_FREE_NULLIFY(endpoint->password);
    M_KII_FREE_NULLIFY(endpoint->username);
    M_KII_FREE_NULLIFY(endpoint->host);
    /* TODO: confirm spec. port is not included? */
    /* M_KII_FREE_NULLIFY(endpoint->port); */
    M_KII_FREE_NULLIFY(endpoint->topic);
    M_KII_FREE_NULLIFY(endpoint);
}

void kii_dispose_kii_char(kii_char_t* char_ptr)
{
    M_KII_FREE_NULLIFY(char_ptr);
}

void kii_json_decref(kii_json_t* json)
{
    json_decref(json);
}

static size_t callbackWrite(char* ptr,
                            size_t size,
                            size_t nmemb,
                            char** respData)
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
        size_t newSize = lastLen + dataLen + 1;
        char* concat = kii_realloc(*respData, newSize);
        if (concat == NULL) {
            return 0;
        }
        kii_strcat(concat, ptr);
        concat[newSize] = '\0';
        if (concat != *respData) {
            M_KII_FREE_NULLIFY(*respData);
        }
        *respData = concat;
    }
    return dataLen;
}

static size_t callback_read(
        char *buffer,
        size_t size,
        size_t nitems,
        void *instream)
{
    prv_kii_http_put_data* put_data = (prv_kii_http_put_data*)instream;
    size_t rest_size = put_data->length - put_data->position;
    size_t max_size = size * nitems;
    size_t actual_size = rest_size < max_size ? rest_size : max_size;
    if (actual_size <= 0) {
        return (curl_off_t)0;
    }
    kii_memcpy(buffer, &put_data->request_body[put_data->position],
            actual_size);
    put_data->position += actual_size;
    return (curl_off_t)actual_size;
}

size_t callback_header(
        char *buffer,
        size_t size,
        size_t nitems,
        void *userdata)
{
    /* TODO: implement me. */
    return size * nitems;
}

char* prv_new_header_string(const char* key, const char* value)
{
    size_t len = kii_strlen(key) + kii_strlen(":") + kii_strlen(value) +1;
    char* val = kii_malloc(len);
    kii_memset(val, '\0', len);
    kii_sprintf(val, "%s:%s", key, value);
    return val;
}

char* prv_new_auth_header_string(const char* access_token)
{
    size_t len = kii_strlen("authorization: bearer ")
        + kii_strlen(access_token);
    char* ret = malloc(len);
    kii_memset(ret, '\0', len);
    kii_sprintf(ret, "authorization: bearer %s", access_token);
    return ret;
}

typedef enum {
    POST,
    PUT,
    PATCH,
    DELETE,
    GET
} prv_kii_req_method_t;

kii_error_code_t prv_execute_curl(CURL* curl,
                                  const char* url,
                                  prv_kii_req_method_t method,
                                  const char* request_body,
                                  struct curl_slist* request_headers,
                                  char** response_body,
                                  json_t** response_headers,
                                  kii_error_t** error)
{
    char* respHeaderData = NULL;
    prv_kii_http_put_data put_data; /* data container for HTTP PUT method. */
    CURLcode curlCode = CURLE_COULDNT_CONNECT; /* set error code as default. */

    M_KII_ASSERT(curl != NULL);
    M_KII_ASSERT(url != NULL);
    M_KII_ASSERT(request_headers != NULL);
    M_KII_ASSERT(error != NULL);

    switch (method) {
        case POST:
            if (request_body != NULL) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
            }
            break;
        case PUT:
            curl_easy_setopt(curl, CURLOPT_PUT, 1L);
            if (request_body != NULL) {
                put_data.request_body = request_body;
                put_data.position = 0;
                put_data.length = kii_strlen(request_body);
                curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
                curl_easy_setopt(curl, CURLOPT_READFUNCTION, callback_read);
                curl_easy_setopt(curl, CURLOPT_READDATA, (void*)&put_data);
                curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                        (curl_off_t)put_data.length);
            }
            break;
        case PATCH:
            request_headers = curl_slist_append(request_headers,
                    "X-HTTP-METHOD-OVERRIDE: PATCH");
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
        default:
            M_KII_ASSERT(0); /* programing error */
            return KIIE_FAIL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, callback_header);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &respHeaderData);

    curlCode = curl_easy_perform(curl);
    if (curlCode != CURLE_OK) {
        *error = prv_construct_kii_error(0, KII_ECODE_CONNECTION);
        return KIIE_FAIL;
    } else {
        long respCode = 0;
        M_KII_DEBUG(prv_log("response: %s", *response_body));
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respCode);
        if ((200 <= respCode) && (respCode < 300)) {
            return KIIE_OK;
        } else {
            char* error_code = NULL;
            json_error_t jErr;
            json_t* errJson = json_loads(*response_body, 0, &jErr);
            if (errJson != NULL) {
                json_t* eCode = json_object_get(errJson, "errorCode");
                if (eCode != NULL) {
                    error_code = json_dumps(eCode, JSON_ENCODE_ANY);
                }
                json_decref(errJson);
            }
            *error = prv_construct_kii_error((int)respCode, error_code);
            if (error_code != NULL) {
                M_KII_FREE_NULLIFY(error_code);
            }
            return KIIE_FAIL;
        }
    }
}


kii_error_code_t kii_register_thing(kii_app_t app,
                                    const kii_char_t* vendor_thing_id,
                                    const kii_char_t* thing_password,
                                    const kii_char_t* opt_thing_type,
                                    const kii_json_t* user_data,
                                    kii_char_t** out_access_token)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    char reqUrl[1024]; /* TODO: calcurate length and alloc minimum length. */
    struct curl_slist* headers = NULL;
    char* appIdHdr = NULL;
    char* appkeyHdr = NULL;
    char* contentTypeHdr = NULL;
    json_t* reqJson = NULL;
    char* reqStr = NULL;
    char* respData = NULL;
    CURLcode curlRet;
    long respCode = 0;
    kii_error_t* err = NULL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(vendor_thing_id != NULL);
    M_KII_ASSERT(thing_password != NULL);

    /* prepare URL */
    kii_memset(reqUrl, '\0', sizeof(reqUrl));
    kii_sprintf(reqUrl, "%s/apps/%s/things", pApp->site_url, pApp->app_id);
    
    /* prepare headers */
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    contentTypeHdr = prv_new_header_string("content-type",
                                                 "application/vnd.kii.ThingRegistrationAndAuthorizationRequest+json");
    headers = curl_slist_append(headers, appIdHdr);
    headers = curl_slist_append(headers, appkeyHdr);
    headers = curl_slist_append(headers, contentTypeHdr);
    
    /* prepare request data */
    if (user_data != NULL) {
        reqJson = json_deep_copy((kii_json_t*)user_data);
    } else {
        reqJson = json_object();
    }
    json_object_set_new(reqJson, "_vendorThingID",
                        json_string(vendor_thing_id));
    json_object_set_new(reqJson, "_password",
                        json_string(thing_password));
    if (opt_thing_type != NULL && kii_strlen(opt_thing_type) > 0) {
        json_object_set_new(reqJson, "_thingType",
                            json_string(opt_thing_type));
    }
    reqStr = json_dumps(reqJson, 0);
    kii_json_decref(reqJson);

    curl_easy_setopt(pApp->curl_easy, CURLOPT_URL, reqUrl);
    curl_easy_setopt(pApp->curl_easy, CURLOPT_POSTFIELDS, reqStr);
    curl_easy_setopt(pApp->curl_easy, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(pApp->curl_easy, CURLOPT_WRITEFUNCTION, callbackWrite);
    curl_easy_setopt(pApp->curl_easy, CURLOPT_WRITEDATA, &respData);
    
    err = kii_malloc(sizeof(kii_error_t));
    err->error_code = NULL; /* TODO: create private kii_error_t initializer. */

    curlRet = curl_easy_perform(pApp->curl_easy);
    if (curlRet != CURLE_OK) {
        err->status_code = 0;
        err->error_code = kii_strdup(KII_ECODE_CONNECTION);
        prv_kii_set_error(pApp, err);
        ret = KIIE_FAIL;
        goto ON_EXIT;
    }

    /* Check response code */
    curl_easy_getinfo(pApp->curl_easy, CURLINFO_RESPONSE_CODE, &respCode);
    if ((200 <= respCode) && (respCode < 300)) {
        json_error_t jErr;
        json_t* respJson = NULL;
        M_KII_DEBUG(prv_log("response: %s", respData));
        respJson = json_loads(respData, 0, &jErr);
        if (respJson != NULL) {
            json_t* accessTokenJson = json_object_get(respJson, "_accessToken");
            if (out_access_token != NULL) {
                char* temp = json_dumps(accessTokenJson, JSON_ENCODE_ANY);
                *out_access_token = temp;
            } else {
                err->status_code = (int)respCode;
                err->error_code = kii_strdup(KII_ECODE_PARSE);
                ret = KIIE_FAIL;
                goto ON_EXIT;
            }
            kii_json_decref(respJson);
        }
        prv_kii_set_error(pApp, NULL);
        ret = KIIE_OK;
        goto ON_EXIT;
    } else {
        json_error_t jErr;
        json_t* errJson = NULL;
        M_KII_DEBUG(prv_log("response: %s", respData));
        err->status_code = (int)respCode;
        err->error_code = kii_strdup("");
        errJson = json_loads(respData, 0, &jErr);
        if (errJson != NULL) {
            json_t* eCode = json_object_get(errJson, "errorCode");
            if (eCode != NULL) {
                err->error_code = json_dumps(eCode, JSON_ENCODE_ANY);
            }
            json_decref(errJson);
        }
        prv_kii_set_error(pApp, err);
        ret = KIIE_FAIL;
        goto ON_EXIT;
    }
    
ON_EXIT:
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(contentTypeHdr);
    curl_slist_free_all(headers);
    M_KII_FREE_NULLIFY(reqStr);
    M_KII_FREE_NULLIFY(respData);

    return ret;
}

kii_bucket_t kii_init_thing_bucket(const kii_char_t* vendor_thing_id,
                                   const kii_char_t* bucket_name)
{
    /* TODO: implement it. */
    return NULL;
}

kii_error_code_t kii_create_new_object(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_bucket_t bucket,
                                       const kii_json_t* contents,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_create_new_object_with_id(kii_app_t app,
                                               const kii_char_t* access_token,
                                               const kii_bucket_t bucket,
                                               const kii_char_t* object_id,
                                               const kii_json_t* contents,
                                               kii_char_t** out_etag)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_patch_object(kii_app_t app,
                                  const kii_char_t* access_token,
                                  const kii_bucket_t bucket,
                                  const kii_char_t* object_id,
                                  const kii_json_t* patch,
                                  const kii_bool_t force_update,
                                  const kii_char_t* opt_etag,
                                  kii_char_t** out_etag)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_replace_object(kii_app_t app,
                                    const kii_char_t* access_token,
                                    const kii_bucket_t bucket,
                                    const kii_char_t* object_id,
                                    const kii_json_t* replace_contents,
                                    const kii_bool_t force_update,
                                    const kii_char_t* opt_etag,
                                    kii_char_t** out_etag)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_get_object(kii_app_t app,
                                const kii_char_t* access_token,
                                const kii_bucket_t bucket,
                                const kii_char_t* object_id,
                                const kii_json_t** out_contents)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_delete_object(kii_app_t app,
                                   const kii_char_t* access_token,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_subscribe_bucket(kii_app_t app,
                                      const kii_char_t* access_token,
                                      const kii_bucket_t bucket)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_unsubscribe_bucket(kii_app_t app,
                                        const kii_char_t* access_token,
                                        const kii_bucket_t bucket)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_is_bucket_subscribed(kii_app_t app,
                                          const kii_char_t* access_token,
                                          const kii_bucket_t bucket,
                                          kii_bool_t* out_is_subscribed)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_topic_t kii_init_thing_topic(const kii_char_t* vendor_thing_id,
                                 const kii_char_t* topic_name)
{
    /* TODO: implement it. */
    return NULL;
}

kii_error_code_t kii_subscribe_topic(kii_app_t app,
                                     const kii_char_t* access_token,
                                     const kii_topic_t topic)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_unsubscribe_topic(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_topic_t topic)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_is_topic_subscribed(kii_app_t app,
                                         const kii_char_t* access_token,
                                         const kii_topic_t topic,
                                         kii_bool_t* out_is_subscribed)
{
    /* TODO: implement it. */
    return KIIE_FAIL;
}

kii_error_code_t kii_install_thing_push(kii_app_t app,
                                        const kii_char_t* access_token,
                                        kii_bool_t development,
                                        kii_char_t** out_installation_id)
{
    kii_char_t* url = NULL;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    json_t* reqBodyJson = NULL;
    kii_char_t* reqBodyStr = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    json_t* respBodyJson = NULL;
    kii_error_t* error = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* contentTypeHdr = NULL;
    kii_char_t* authHdr = NULL;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;
    json_error_t jErr;
    
    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(out_installation_id != NULL);

    /* Prepare URL */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
                        "installations",
                        NULL);
    
    /* Prepare body */
    reqBodyJson = json_object();
    json_object_set_new(reqBodyJson, "deviceType", json_string("MQTT"));
    json_object_set_new(reqBodyJson, "development", json_boolean(development));
    reqBodyStr = json_dumps(reqBodyJson, 0);
    kii_json_decref(reqBodyJson);

    /* Prepare headers*/
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    contentTypeHdr = prv_new_header_string("content-type",
                                           "application/vnd.kii.InstallationCreationRequest+json");
    authHdr = prv_new_auth_header_string(access_token);
    
    reqHeaders = curl_slist_append(reqHeaders, appIdHdr);
    reqHeaders = curl_slist_append(reqHeaders, appkeyHdr);
    reqHeaders = curl_slist_append(reqHeaders, contentTypeHdr);
    reqHeaders = curl_slist_append(reqHeaders, authHdr);

    exeCurlRet = prv_execute_curl(pApp->curl_easy,
                                  url,
                                  POST,
                                  reqBodyStr,
                                  reqHeaders,
                                  &respBodyStr,
                                  NULL,
                                  &error);
    if (exeCurlRet != KIIE_OK) {
        prv_kii_set_error(app, error);
        ret = KIIE_FAIL;
        goto ON_EXIT;
    }

    /* Parse body */
    respBodyJson = json_loads(respBodyStr, 0, &jErr);
    if (respBodyJson != NULL) {
        json_t* installIDJson = NULL;
        installIDJson = json_object_get(respBodyJson, "installationID");
        if (installIDJson != NULL) {
            *out_installation_id = kii_strdup(json_string_value(installIDJson));
            ret = KIIE_OK;
            goto ON_EXIT;
        }
    }
    if (*out_installation_id == NULL) { /* parse error */
        error = prv_construct_kii_error(0, KII_ECODE_PARSE);
        prv_kii_set_error(app, error);
        ret = KIIE_FAIL;
        goto ON_EXIT;
    }

ON_EXIT:
    kii_json_decref(respBodyJson);
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(reqBodyStr);
    M_KII_FREE_NULLIFY(respBodyStr);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    curl_slist_free_all(reqHeaders);

    return ret;
}

kii_error_code_t kii_get_mqtt_endpoint(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_char_t* installation_id,
                                       kii_mqtt_endpoint_t** out_endpoint,
                                       kii_uint_t* out_retry_after_in_second)
{
    kii_char_t* url = NULL;
    prv_kii_app_t* pApp = (prv_kii_app_t*) app;
    struct curl_slist* reqHeaders = NULL;
    char* appIdHdr = NULL;
    char* appkeyHdr = NULL;
    char* authHdr = NULL;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_char_t* respBodyStr = NULL;
    json_t* respBodyJson = NULL;
    kii_error_t* error = NULL;
    kii_error_code_t ret = KIIE_FAIL;
    json_error_t jErr;
    json_t* userNameJson = NULL;
    json_t* passwordJson = NULL;
    json_t* mqttTopicJson = NULL;
    json_t* hostJson = NULL;
    json_t* mqttTtlJson = NULL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(out_endpoint != NULL);

    /* Prepare URL */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
                        "installations",
                        installation_id,
                        "mqtt-endpoint",
                        NULL);

    M_KII_DEBUG(prv_log("mqtt endpoint url: %s", url));
    /* Prepare Headers */
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    reqHeaders = curl_slist_append(reqHeaders, appIdHdr);
    reqHeaders = curl_slist_append(reqHeaders, appkeyHdr);
    reqHeaders = curl_slist_append(reqHeaders, authHdr);

    exeCurlRet = prv_execute_curl(pApp->curl_easy,
                                  url,
                                  GET,
                                  NULL,
                                  reqHeaders,
                                  &respBodyStr,
                                  NULL,
                                  &error);
    if (exeCurlRet != KIIE_OK) {
        if (error->status_code == 503) {
            json_t* retryAfterJson = NULL;
            respBodyJson = json_loads(respBodyStr, 0, &jErr);
            if (respBodyJson) {
                int retryAfterInt = 0;
                retryAfterJson = json_object_get(respBodyJson, "retryAfter");
                retryAfterInt = (int)json_integer_value(retryAfterJson);
                if (retryAfterInt) {
                    *out_retry_after_in_second = retryAfterInt;
                }
            }
        }
        prv_kii_set_error(app, error);
        ret = KIIE_FAIL;
        goto ON_EXIT;
    }

    /* Parse body */
    respBodyJson = json_loads(respBodyStr, 0, &jErr);
    if (respBodyJson != NULL) {
        userNameJson = json_object_get(respBodyJson, "username");
        passwordJson = json_object_get(respBodyJson, "password");
        mqttTopicJson = json_object_get(respBodyJson, "mqttTopic");
        hostJson = json_object_get(respBodyJson, "host");
        mqttTtlJson = json_object_get(respBodyJson, "X-MQTT-TTL");
        if (userNameJson == NULL || passwordJson == NULL ||
            mqttTopicJson == NULL || hostJson == NULL || mqttTtlJson == NULL) {
            error = prv_construct_kii_error(0, KII_ECODE_PARSE);
            prv_kii_set_error(app, error);
            ret = KIIE_FAIL;
            goto ON_EXIT;
        }
        *out_endpoint = kii_malloc(sizeof(kii_mqtt_endpoint_t));
        (*out_endpoint)->username = kii_strdup(json_string_value(userNameJson));
        (*out_endpoint)->password = kii_strdup(json_string_value(passwordJson));
        (*out_endpoint)->topic = kii_strdup(json_string_value(mqttTopicJson));
        (*out_endpoint)->host = kii_strdup(json_string_value(hostJson));
        (*out_endpoint)->ttl = (kii_ulong_t)json_integer_value(mqttTtlJson);
        ret = KIIE_OK;
        goto ON_EXIT;
    }
    /* if body not present : parse error */
    error = prv_construct_kii_error(0, KII_ECODE_PARSE);
    prv_kii_set_error(app, error);
    ret = KIIE_FAIL;
    goto ON_EXIT;

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    M_KII_FREE_NULLIFY(respBodyStr);
    curl_slist_free_all(reqHeaders);
    kii_json_decref(respBodyJson);
    return ret;
}
