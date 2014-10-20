//
//  kii_cloud.c
//  KiiThingSDK
//
//  Copyright (c) 2014 Kii. All rights reserved.
//

#include "curl.h"
#include "kii_cloud.h"
#include "kii_logger.h"
#include "kii_libc.h"

kii_error_code_t kii_global_init(void)
{
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    return ((r == CURLE_OK) ? KIIE_OK : KIIE_FAIL);
}

void kii_global_clenaup(void)
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
    char* thing_vendor_id;
    char* bucket_name;
} prv_kii_bucket_t;

typedef struct prv_kii_topic_t {
    char* thing_vendor_id;
    char* topic_name;
} prv_kii_topic_t;

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
    M_KII_FREE_NULLIFY(pBucket->thing_vendor_id);
    M_KII_FREE_NULLIFY(bucket);
}

void kii_dispose_topic(kii_topic_t topic)
{
    prv_kii_topic_t* pTopic = (prv_kii_topic_t*) topic;
    M_KII_FREE_NULLIFY(pTopic->topic_name);
    M_KII_FREE_NULLIFY(pTopic->thing_vendor_id);
    M_KII_FREE_NULLIFY(topic);
}

void kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t* endpoint)
{
    M_KII_FREE_NULLIFY(endpoint->password);
    M_KII_FREE_NULLIFY(endpoint->username);
    M_KII_FREE_NULLIFY(endpoint->host);
    M_KII_FREE_NULLIFY(endpoint->port);
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
    if (*respData == NULL) { /* First time. */
        *respData = kii_strdup(ptr);
    } else {
        size_t lastLen = kii_strlen(*respData);
        size_t newSize = lastLen + size + 1;
        char* concat = malloc(newSize);
        kii_memset(concat, '\0', newSize);
        kii_strcat(concat, *respData);
        kii_strcat(concat, ptr);
        M_KII_FREE_NULLIFY(*respData);
        *respData = concat;
    }
    return dataLen;
}

char* prv_new_header_string(const char* key, const char* value)
{
    size_t len = kii_strlen(key) + kii_strlen(":") + kii_strlen(value) +1;
    char* val = kii_malloc(len);
    kii_memset(val, '\0', len);
    kii_sprintf(val, "%s:%s", key, value);
    return val;
}

typedef enum {
    POST,
    PUT,
    PATCH,
    DELETE
} prv_kii_req_method_t;

kii_error_code_t prv_execute_curl(CURL* curl,
                                  const char* url,
                                  prv_kii_req_method_t method,
                                  const char* request_body,
                                  const struct curl_slist* headers,
                                  char** response_body,
                                  kii_error_t** error)
{
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackWrite);
    char* respData = NULL;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respData);
    
    
    return KIIE_FAIL;
}


kii_error_code_t kii_register_thing(kii_app_t app,
                                    const kii_char_t* thing_vendor_id,
                                    const kii_char_t* thing_password,
                                    const kii_char_t* opt_thing_type,
                                    const kii_json_t* user_data,
                                    kii_char_t** out_access_token)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(thing_vendor_id != NULL);
    M_KII_ASSERT(thing_password != NULL);

    /* prepare URL */
    char reqUrl[1024]; // TODO: calcurate length and alloc minimum length.
    kii_memset(reqUrl, '\0', sizeof(reqUrl));
    kii_sprintf(reqUrl, "%s/apps/%s/things", pApp->site_url, pApp->app_id);
    
    /* prepare headers */
    struct curl_slist* headers = NULL;
    char* appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    char* appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    char* contentTypeHdr = prv_new_header_string("content-type",
                                                 "application/vnd.kii.ThingRegistrationAndAuthorizationRequest+json");
    headers = curl_slist_append(headers, appIdHdr);
    headers = curl_slist_append(headers, appkeyHdr);
    headers = curl_slist_append(headers, contentTypeHdr);
    
    /* prepare request data */
    json_t* reqJson = NULL;
    if (user_data != NULL) {
        reqJson = json_deep_copy((kii_json_t*)user_data);
    } else {
        reqJson = json_object();
    }
    json_object_set_new(reqJson, "_vendorThingID",
                        json_string(thing_vendor_id));
    json_object_set_new(reqJson, "_password",
                        json_string(thing_password));
    if (opt_thing_type != NULL && kii_strlen(opt_thing_type) > 0) {
        json_object_set_new(reqJson, "_thingType",
                            json_string(opt_thing_type));
    }
    char* reqStr = json_dumps(reqJson, 0);
    kii_json_decref(reqJson);

    char* respData = NULL;
    curl_easy_setopt(pApp->curl_easy, CURLOPT_URL, reqUrl);
    curl_easy_setopt(pApp->curl_easy, CURLOPT_POSTFIELDS, reqStr);
    curl_easy_setopt(pApp->curl_easy, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(pApp->curl_easy, CURLOPT_WRITEFUNCTION, callbackWrite);
    curl_easy_setopt(pApp->curl_easy, CURLOPT_WRITEDATA, &respData);
    
    kii_error_t* err = kii_malloc(sizeof(kii_error_t));
    err->error_code = NULL; // TODO: create private kii_error_t initializer.
    kii_error_code_t ret = KIIE_FAIL;

    CURLcode curlRet = curl_easy_perform(pApp->curl_easy);
    if (curlRet != CURLE_OK) {
        err->status_code = 0;
        err->error_code = kii_strdup(KII_ECODE_CONNECTION);
        prv_kii_set_error(pApp, err);
        ret = KIIE_FAIL;
        goto ON_EXIT;
    }

    // Check response code
    long respCode = 0;
    curl_easy_getinfo(pApp->curl_easy, CURLINFO_RESPONSE_CODE, &respCode);
    if ((200 <= respCode) && (respCode < 300)) {
        M_KII_LOG("response: %s", respData);
        json_error_t jErr;
        json_t* respJson = json_loads(respData, 0, &jErr);
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
        M_KII_LOG("response: %s", respData);
        err->status_code = (int)respCode;
        err->error_code = kii_strdup("");
        json_error_t jErr;
        json_t* errJson = json_loads(respData, 0, &jErr);
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

kii_bucket_t kii_init_thing_bucket(const kii_char_t* thing_vendor_id,
                                   const kii_char_t* bucket_name)
{
    // TODO: implement it.
    return NULL;
}

kii_error_code_t kii_create_new_object(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_bucket_t bucket,
                                       const kii_json_t* contents,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_create_new_object_with_id(kii_app_t app,
                                               const kii_char_t* access_token,
                                               const kii_bucket_t bucket,
                                               const kii_char_t* object_id,
                                               const kii_json_t* contents,
                                               kii_char_t** out_etag)
{
    // TODO: implement it.
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
    // TODO: implement it.
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
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_get_object(kii_app_t app,
                                const kii_char_t* access_token,
                                const kii_bucket_t bucket,
                                const kii_char_t* object_id,
                                const kii_json_t** out_contents)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_delete_object(kii_app_t app,
                                   const kii_char_t* access_token,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_subscribe_bucket(kii_app_t app,
                                      const kii_char_t* access_token,
                                      const kii_bucket_t bucket)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_unsubscribe_bucket(kii_app_t app,
                                        const kii_char_t* access_token,
                                        const kii_bucket_t bucket)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_is_bucket_subscribed(kii_app_t app,
                                          const kii_char_t* access_token,
                                          const kii_bucket_t bucket,
                                          kii_bool_t* out_is_subscribed)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_topic_t kii_init_thing_topic(const kii_char_t* thing_vendor_id,
                                 const kii_char_t* topic_name)
{
    // TODO: implement it.
    return NULL;
}

kii_error_code_t kii_subscribe_topic(kii_app_t app,
                                     const kii_char_t* access_token,
                                     const kii_topic_t topic)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_unsubscribe_topic(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_topic_t topic)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_is_topic_subscribed(kii_app_t app,
                                         const kii_char_t* access_token,
                                         const kii_topic_t topic,
                                         kii_bool_t* out_is_subscribed)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_install_thing_push(kii_app_t app,
                                        const kii_char_t* access_token,
                                        kii_char_t** out_installation_id)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_get_mqtt_endpoint(kii_app_t app,
                                       const kii_char_t* installation_id,
                                       const kii_char_t* access_token,
                                       kii_mqtt_endpoint_t** out_endpoint,
                                       kii_uint_t* out_retry_after_in_second)
{
    // TODO: implement it.
    return KIIE_FAIL;
}
