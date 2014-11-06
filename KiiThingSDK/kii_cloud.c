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
    kii_char_t* app_id;
    kii_char_t* app_key;
    kii_char_t* site_url;
    CURL* curl_easy;
    kii_error_code_t last_result;
    kii_error_t last_error;
} prv_kii_app_t;

typedef struct prv_kii_thing_t {
    kii_char_t* kii_thing_id; /* thing id assigned by kii cloud */
} prv_kii_thing_t;

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

typedef struct prv_kii_bucket_t {
    kii_char_t* kii_thing_id;
    kii_char_t* bucket_name;
} prv_kii_bucket_t;

typedef struct prv_kii_topic_t {
    kii_char_t* kii_thing_id;
    kii_char_t* topic_name;
} prv_kii_topic_t;

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
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    switch (pApp->last_result) {
        case KIIE_OK:
        case KIIE_LOWMEMORY:
        case KIIE_RESPWRITE:
            return NULL;
        case KIIE_FAIL:
            return &(pApp->last_error);
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
    if (new_error != NULL) {
        prv_kii_set_info_in_error(&(app->last_error),
              new_error->status_code, new_error->error_code);
    }
}

void kii_dispose_app(kii_app_t app)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    M_KII_FREE_NULLIFY(pApp->app_id);
    M_KII_FREE_NULLIFY(pApp->app_key);
    M_KII_FREE_NULLIFY(pApp->site_url);
    curl_easy_cleanup(pApp->curl_easy);
    pApp->curl_easy = NULL;
    M_KII_FREE_NULLIFY(app);
}

void kii_dispose_bucket(kii_bucket_t bucket)
{
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*) bucket;
    M_KII_FREE_NULLIFY(pBucket->bucket_name);
    M_KII_FREE_NULLIFY(pBucket->kii_thing_id);
    M_KII_FREE_NULLIFY(bucket);
}

void kii_dispose_topic(kii_topic_t topic)
{
    prv_kii_topic_t* pTopic = (prv_kii_topic_t*) topic;
    M_KII_FREE_NULLIFY(pTopic->topic_name);
    M_KII_FREE_NULLIFY(pTopic->kii_thing_id);
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
        if (concat != *respData) {
            M_KII_FREE_NULLIFY(*respData);
        }
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
    kii_char_t* line = kii_malloc(len + 1);

    M_KII_ASSERT(userdata != NULL);

    if (line == NULL) {
        return 0;
    }

    {
        int i = 0;
        kii_memcpy(line, buffer, len);
        line[len] = '\0';
        M_KII_ASSERT(prv_log_no_LF("resp header: %s", line));
        /* Field name becomes upper case. */
        for (i = 0; line[i] != '\0' && line[i] != ':'; ++i) {
            line[i] = (char)kii_tolower(line[i]);
        }
    }

    /* check http header name. */
    if (kii_strncmp(line, ETAG, kii_strlen(ETAG)) == 0) {
        json_t** json = (json_t**)userdata;
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
        }
        json_object_set_new(*json, ETAG, json_string(value));
    }

    M_KII_FREE_NULLIFY(line);
    return len;
}

kii_char_t* prv_new_header_string(const kii_char_t* key, const kii_char_t* value)
{
    size_t len = kii_strlen(key) + kii_strlen(":") + kii_strlen(value);
    kii_char_t* val = kii_malloc(len + 1);

    if (val == NULL) {
        return NULL;
    }

    val[len] = '\0';
    kii_sprintf(val, "%s:%s", key, value);
    return val;
}

kii_char_t* prv_new_auth_header_string(const kii_char_t* access_token)
{
    size_t len = kii_strlen("authorization: bearer ")
        + kii_strlen(access_token);
    kii_char_t* ret = malloc(len + 1);
    ret[len] = '\0';
    kii_sprintf(ret, "authorization: bearer %s", access_token);
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

static void prv_log_req_heder(struct curl_slist* header)
{
    while (header != NULL) {
        prv_log("req header: %s", header->data);
        header = header->next;
    }
}

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

    M_KII_DEBUG(prv_log_req_heder(request_headers));
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
                json_error_t jErr;
                json_t* errJson = json_loads(*response_body, 0, &jErr);
                if (errJson != NULL) {
                    error_code = json_string_value(errJson);
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
    prv_kii_thing_t* pThing = (prv_kii_thing_t*) thing;
    M_KII_FREE_NULLIFY(pThing->kii_thing_id);
    M_KII_FREE_NULLIFY(thing);
}

kii_char_t* kii_thing_serialize(const kii_thing_t thing)
{
    prv_kii_thing_t* pThing = (prv_kii_thing_t*)thing;
    kii_char_t* ret = NULL;

    M_KII_ASSERT(pThing != NULL);
    M_KII_ASSERT(pThing->kii_thing_id != NULL);
    M_KII_ASSERT(kii_strlen(pThing->kii_thing_id) > 0);

    ret = kii_strdup(pThing->kii_thing_id);
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
                                    const kii_json_t* user_data,
                                    kii_thing_t* out_thing,
                                    kii_char_t** out_access_token)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* contentTypeHdr = NULL;
    json_t* reqJson = NULL;
    kii_char_t* reqStr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(vendor_thing_id != NULL);
    M_KII_ASSERT(thing_password != NULL);
    M_KII_ASSERT(out_thing !=NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(pApp->site_url, "apps", pApp->app_id, "things",
            NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    /* prepare headers */
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    contentTypeHdr = prv_new_header_string("content-type",
                                                 "application/vnd.kii.ThingRegistrationAndAuthorizationRequest+json");
    if (appIdHdr == NULL ||
            appkeyHdr == NULL ||
            contentTypeHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    headers = prv_curl_slist_create(appIdHdr, appkeyHdr, contentTypeHdr, NULL);
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
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

    exeCurlRet = prv_execute_curl(pApp->curl_easy, reqUrl, POST,
            reqStr, headers, &respCode, &respData, NULL, &err);
    if (exeCurlRet != KIIE_OK) {
        ret = exeCurlRet;
        goto ON_EXIT;
    }

    /* Check response code */
    {
        json_error_t jErr;
        respJson = json_loads(respData, 0, &jErr);

        if (respJson != NULL) {
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
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(contentTypeHdr);
    curl_slist_free_all(headers);
    M_KII_FREE_NULLIFY(reqStr);
    M_KII_FREE_NULLIFY(respData);
    M_KII_FREE_NULLIFY(reqUrl);
    kii_json_decref(respJson);

    prv_kii_set_last_error(pApp, ret, &err);

    return ret;
}


kii_bucket_t kii_init_thing_bucket(const kii_thing_t thing,
                                   const kii_char_t* bucket_name)
{
    prv_kii_thing_t* pThing = (prv_kii_thing_t*)thing;
    prv_kii_bucket_t* retval = kii_malloc(sizeof(prv_kii_bucket_t));
    kii_char_t* thing_id = kii_strdup(pThing->kii_thing_id);
    kii_char_t* bucket_name_str = kii_strdup(bucket_name);

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
                                       const kii_json_t* contents,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* authHdr = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* contentTypeHdr = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(pApp != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(pBucket != NULL);
    M_KII_ASSERT(kii_strlen(pBucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(pBucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(contents != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(pApp->site_url, "apps", pApp->app_id, "things",
            pBucket->kii_thing_id, "buckets", pBucket->bucket_name,
            "objects", NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    authHdr = prv_new_auth_header_string(access_token);
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    contentTypeHdr = prv_new_header_string("content-type",
            "application/json");
    if (authHdr == NULL || appIdHdr == NULL || appkeyHdr == NULL ||
            contentTypeHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    headers = prv_curl_slist_create(authHdr, appIdHdr, appkeyHdr,
            contentTypeHdr, NULL);
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqStr = json_dumps(contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(pApp->curl_easy, reqUrl, POST,
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
        if (respJson != NULL) {
            const kii_char_t* objectID = json_string_value(json_object_get(respJson,
                    "objectID"));
            if (objectID != NULL) {
                *out_object_id = kii_strdup(objectID);
                if (*out_object_id == NULL) {
                    ret = KIIE_LOWMEMORY;
                    goto ON_EXIT;
                }
                ret = KIIE_OK;
            } else {
                prv_kii_set_info_in_error(&err, (int)respCode, KII_ECODE_PARSE);
                ret = KIIE_FAIL;
            }
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
    M_KII_FREE_NULLIFY(authHdr);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(contentTypeHdr);
    M_KII_FREE_NULLIFY(reqStr);
    kii_json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);
    kii_json_decref(respJson);

    prv_kii_set_last_error(pApp, ret, &err);

    return ret;
}

kii_error_code_t kii_create_new_object_with_id(kii_app_t app,
                                               const kii_char_t* access_token,
                                               const kii_bucket_t bucket,
                                               const kii_char_t* object_id,
                                               const kii_json_t* contents,
                                               kii_char_t** out_etag)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    kii_char_t* reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* authHdr = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* contentTypeHdr = NULL;
    kii_char_t* ifNoneMatchHdr = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t exeCurlRet = KIIE_FAIL;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(pApp != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(pBucket != NULL);
    M_KII_ASSERT(kii_strlen(pBucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(pBucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(contents != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(pApp->site_url, "apps", pApp->app_id, "things",
            pBucket->kii_thing_id, "buckets", pBucket->bucket_name,
            "objects", object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    authHdr = prv_new_auth_header_string(access_token);
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    contentTypeHdr = prv_new_header_string("content-type",
            "application/json");
    ifNoneMatchHdr = prv_new_header_string("if-none-match", "*");
    if (authHdr == NULL || appIdHdr == NULL || appkeyHdr == NULL ||
        contentTypeHdr == NULL || ifNoneMatchHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    headers = prv_curl_slist_create(authHdr, appIdHdr, appkeyHdr,
            contentTypeHdr, ifNoneMatchHdr, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqStr = json_dumps(contents, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(pApp->curl_easy, reqUrl, PUT,
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
    M_KII_FREE_NULLIFY(authHdr);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(contentTypeHdr);
    kii_dispose_kii_char(ifNoneMatchHdr);
    M_KII_FREE_NULLIFY(reqStr);
    kii_json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);

    prv_kii_set_last_error(pApp, ret, &err);

    return ret;
}

kii_error_code_t kii_patch_object(kii_app_t app,
                                  const kii_char_t* access_token,
                                  const kii_bucket_t bucket,
                                  const kii_char_t* object_id,
                                  const kii_json_t* patch,
                                  const kii_char_t* opt_etag,
                                  kii_char_t** out_etag)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* authHdr = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* contentTypeHdr = NULL;
    kii_char_t* matchHdr = NULL;
    kii_char_t* reqStr = NULL;
    json_t* respHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respJson = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(pApp != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(pBucket != NULL);
    M_KII_ASSERT(kii_strlen(pBucket->kii_thing_id) > 0);
    M_KII_ASSERT(kii_strlen(pBucket->bucket_name) > 0);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(patch != NULL);
    M_KII_ASSERT(out_etag != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(pApp->site_url, "apps", pApp->app_id, "things",
            pBucket->kii_thing_id, "buckets", pBucket->bucket_name, "objects",
            object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    authHdr = prv_new_auth_header_string(access_token);
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    contentTypeHdr = prv_new_header_string("content-type",
            "application/json");
    matchHdr = (opt_etag == NULL) ?
        prv_new_header_string("If-None-Match", "*") :
        prv_new_header_string("If-Match", opt_etag);
    if (authHdr == NULL || appIdHdr == NULL || appkeyHdr == NULL ||
            contentTypeHdr == NULL || matchHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    headers = prv_curl_slist_create(authHdr, appIdHdr, appkeyHdr,
            contentTypeHdr, matchHdr, NULL);
    if (headers == NULL) {;
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqStr = json_dumps(patch, 0);
    if (reqStr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy, reqUrl, PATCH,
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
    M_KII_FREE_NULLIFY(authHdr);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(contentTypeHdr);
    M_KII_FREE_NULLIFY(reqStr);
    kii_json_decref(respHdr);
    M_KII_FREE_NULLIFY(respData);
    kii_json_decref(respJson);

    prv_kii_set_last_error(pApp, ret, &err);

    return ret;
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
                                kii_json_t** out_contents,
                                kii_char_t** out_etag)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    json_t* respHdr = NULL;
    json_error_t jErr;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(object_id != NULL);
    M_KII_ASSERT(out_contents != NULL);
    M_KII_ASSERT(out_etag != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(pApp->site_url, "apps", pApp->app_id, "things",
            pBucket->kii_thing_id, "buckets", pBucket->bucket_name, "objects",
            object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL ||
            appkeyHdr == NULL ||
            authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    headers = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy, reqUrl, GET,
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
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    M_KII_FREE_NULLIFY(respData);
    kii_json_decref(respHdr);

    prv_kii_set_last_error(pApp, ret, &err);

    return ret;
}

kii_error_code_t kii_delete_object(kii_app_t app,
                                   const kii_char_t* access_token,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id)
{
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    kii_char_t *reqUrl = NULL;
    struct curl_slist* headers = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    long respCode = 0;
    kii_char_t* respData = NULL;
    kii_error_t err;
    kii_error_code_t ret = KIIE_FAIL;

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(kii_strlen(pApp->app_id)>0);
    M_KII_ASSERT(kii_strlen(pApp->app_key)>0);
    M_KII_ASSERT(kii_strlen(pApp->site_url)>0);
    M_KII_ASSERT(pApp->curl_easy != NULL);
    M_KII_ASSERT(bucket != NULL);
    M_KII_ASSERT(object_id != NULL);

    kii_memset(&err, 0, sizeof(kii_error_t));

    /* prepare URL */
    reqUrl = prv_build_url(pApp->site_url, "apps", pApp->app_id, "things",
            pBucket->kii_thing_id, "buckets", pBucket->bucket_name, "objects",
            object_id, NULL);
    if (reqUrl == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    /* prepare headers */
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL ||
            appkeyHdr == NULL ||
            authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    headers = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy, reqUrl, DELETE,
            NULL, headers, &respCode, &respData, NULL, &err);

ON_EXIT:
    M_KII_FREE_NULLIFY(reqUrl);
    curl_slist_free_all(headers);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    M_KII_FREE_NULLIFY(respData);

    prv_kii_set_last_error(pApp, ret, &err);

    return ret;
}

kii_error_code_t kii_subscribe_bucket(kii_app_t app,
                                      const kii_char_t* access_token,
                                      const kii_bucket_t bucket)
{
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    const kii_char_t* thingId = pBucket->kii_thing_id;
    const kii_char_t* bucketName = pBucket->bucket_name;
    kii_char_t* url = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL || appkeyHdr == NULL || authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy,
                           url,
                           POST,
                           NULL,
                           reqHeaders,
                           &respStatus,
                           &respBodyStr,
                           NULL,
                           &error);

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    curl_slist_free_all(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);

    prv_kii_set_last_error(pApp, ret, &error);
    return ret;
}

kii_error_code_t kii_unsubscribe_bucket(kii_app_t app,
                                        const kii_char_t* access_token,
                                        const kii_bucket_t bucket)
{
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    const kii_char_t* thingId = pBucket->kii_thing_id;
    const kii_char_t* bucketName = pBucket->bucket_name;
    kii_char_t* url = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL || appkeyHdr == NULL || authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy,
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
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    curl_slist_free_all(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);

    prv_kii_set_last_error(pApp, ret, &error);
    return ret;
}

kii_error_code_t kii_is_bucket_subscribed(kii_app_t app,
                                          const kii_char_t* access_token,
                                          const kii_bucket_t bucket,
                                          kii_bool_t* out_is_subscribed)
{
    prv_kii_bucket_t* pBucket = (prv_kii_bucket_t*)bucket;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    const kii_char_t* thingId = pBucket->kii_thing_id;
    const kii_char_t* bucketName = pBucket->bucket_name;
    kii_char_t* url = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL || appkeyHdr == NULL || authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy,
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
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    curl_slist_free_all(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);

    prv_kii_set_last_error(pApp, ret, &error);
    return ret;
}

kii_topic_t kii_init_thing_topic(const kii_thing_t thing,
                                 const kii_char_t* topic_name)
{
    prv_kii_topic_t* topic = NULL;
    kii_char_t* tempThingId = NULL;
    kii_char_t* tempTopicName = NULL;
    prv_kii_thing_t* pThing = (prv_kii_thing_t*)thing;
    
    M_KII_ASSERT(pThing->kii_thing_id != NULL);
    M_KII_ASSERT(kii_strlen(pThing->kii_thing_id) > 0);

    topic = kii_malloc(sizeof(prv_kii_topic_t));
    if (topic == NULL) {
        return NULL;
    }

    tempThingId = kii_strdup(pThing->kii_thing_id);
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

kii_error_code_t kii_subscribe_topic(kii_app_t app,
                                     const kii_char_t* access_token,
                                     const kii_topic_t topic)
{
    prv_kii_topic_t* pTopic = (prv_kii_topic_t*)topic;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    const kii_char_t* thingId = pTopic->kii_thing_id;
    const kii_char_t* topicName = pTopic->topic_name;
    kii_char_t* url = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;
    kii_char_t* respBodyStr = NULL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL || appkeyHdr == NULL || authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy,
                     url,
                     POST,
                     NULL,
                     reqHeaders,
                     &respStatus,
                     &respBodyStr,
                     NULL,
                     &error);

ON_EXIT:
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    curl_slist_free_all(reqHeaders);
    M_KII_FREE_NULLIFY(respBodyStr);
    prv_kii_set_last_error(pApp, ret, &error);

    return ret;
}

kii_error_code_t kii_unsubscribe_topic(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_topic_t topic)
{
    prv_kii_topic_t* pTopic = (prv_kii_topic_t*)topic;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    const kii_char_t* thingId = pTopic->kii_thing_id;
    const kii_char_t* topicName = pTopic->topic_name;
    kii_char_t* url = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;
    long respStatus = 0;
    kii_char_t* respBodyStr = NULL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL || appkeyHdr == NULL || authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    reqHeaders = curl_slist_append(reqHeaders, appIdHdr);
    reqHeaders = curl_slist_append(reqHeaders, appkeyHdr);
    reqHeaders = curl_slist_append(reqHeaders, authHdr);

    ret = prv_execute_curl(pApp->curl_easy,
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
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
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
    prv_kii_topic_t* pTopic = (prv_kii_topic_t*)topic;
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    const kii_char_t* thingId = pTopic->kii_thing_id;
    const kii_char_t* topicName = pTopic->topic_name;
    kii_char_t* url = NULL;
    struct curl_slist* reqHeaders = NULL;
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
    long respStatus = 0;
    kii_char_t* respBodyStr = NULL;
    kii_error_t error;
    kii_error_code_t ret = KIIE_FAIL;

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare Url */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL || appkeyHdr == NULL || authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr,
            NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(pApp->curl_easy,
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
    kii_dispose_kii_char(appIdHdr);
    kii_dispose_kii_char(appkeyHdr);
    kii_dispose_kii_char(authHdr);
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
    prv_kii_app_t* pApp = (prv_kii_app_t*)app;
    json_t* reqBodyJson = NULL;
    kii_char_t* reqBodyStr = NULL;
    struct curl_slist* reqHeaders = NULL;
    long respCode = 0;
    kii_char_t* respBodyStr = NULL;
    json_t* respBodyJson = NULL;
    kii_error_t error;
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

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare URL */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
                        "installations",
                        NULL);
    if (url == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }
    
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
    if (appIdHdr == NULL ||
            appkeyHdr == NULL ||
            contentTypeHdr == NULL ||
            authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, contentTypeHdr,
            authHdr, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(pApp->curl_easy,
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
    if (respBodyJson != NULL) {
        json_t* installIDJson = NULL;
        installIDJson = json_object_get(respBodyJson, "installationID");
        if (installIDJson != NULL) {
            *out_installation_id = kii_strdup(json_string_value(installIDJson));
            ret = *out_installation_id != NULL ? KIIE_OK : KIIE_LOWMEMORY;
            goto ON_EXIT;
        }
    }

    prv_kii_set_info_in_error(&error, (int)respCode, KII_ECODE_PARSE);
    ret = KIIE_FAIL;
    goto ON_EXIT;

ON_EXIT:
    kii_json_decref(respBodyJson);
    M_KII_FREE_NULLIFY(url);
    M_KII_FREE_NULLIFY(reqBodyStr);
    M_KII_FREE_NULLIFY(respBodyStr);
    M_KII_FREE_NULLIFY(appIdHdr);
    M_KII_FREE_NULLIFY(appkeyHdr);
    M_KII_FREE_NULLIFY(authHdr);
    curl_slist_free_all(reqHeaders);
    prv_kii_set_last_error(pApp, ret, &error);

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
    kii_char_t* appIdHdr = NULL;
    kii_char_t* appkeyHdr = NULL;
    kii_char_t* authHdr = NULL;
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

    M_KII_ASSERT(app != NULL);
    M_KII_ASSERT(access_token != NULL);
    M_KII_ASSERT(out_endpoint != NULL);

    kii_memset(&error, 0, sizeof(kii_error_t));

    /* Prepare URL */
    url = prv_build_url(pApp->site_url,
                        "apps",
                        pApp->app_id,
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
    appIdHdr = prv_new_header_string("x-kii-appid", pApp->app_id);
    appkeyHdr = prv_new_header_string("x-kii-appkey", pApp->app_key);
    authHdr = prv_new_auth_header_string(access_token);
    if (appIdHdr == NULL ||
            appkeyHdr == NULL ||
            authHdr == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    reqHeaders = prv_curl_slist_create(appIdHdr, appkeyHdr, authHdr, NULL);
    if (reqHeaders == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    exeCurlRet = prv_execute_curl(pApp->curl_easy,
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
            if (respBodyJson) {
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
    if (respBodyJson != NULL) {
        userNameJson = json_object_get(respBodyJson, "username");
        passwordJson = json_object_get(respBodyJson, "password");
        mqttTopicJson = json_object_get(respBodyJson, "mqttTopic");
        hostJson = json_object_get(respBodyJson, "host");
        mqttTtlJson = json_object_get(respBodyJson, "X-MQTT-TTL");
        if (userNameJson == NULL || passwordJson == NULL ||
            mqttTopicJson == NULL || hostJson == NULL || mqttTtlJson == NULL) {
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
            *out_endpoint = endpoint;
        }

        ret = KIIE_OK;
        goto ON_EXIT;
    }
    /* if body not present : parse error */
    prv_kii_set_info_in_error(&error, (int)respCode, KII_ECODE_PARSE);
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
    prv_kii_set_last_error(pApp, ret, &error);

    return ret;
}
