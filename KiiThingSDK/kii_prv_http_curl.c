#ifdef XCODE
#include "curl.h"
#else
#include <curl/curl.h>
#endif

#include "kii_prv_http_adapter.h"
#include "kii_prv_types.h"
#include "kii_prv_utils.h"

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

kii_error_code_t prv_kii_http_delete(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error)
{
    kii_error_code_t ret = KIIE_FAIL;
    struct curl_slist* headers = NULL;

    /* prepare headers */
    headers = prv_common_request_headers(app, access_token, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy, url, DELETE, NULL, headers,
	    response_status_code, response_body, NULL, error);

ON_EXIT:
    curl_slist_free_all(headers);

    return ret;
}

kii_error_code_t prv_kii_http_get(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    kii_error_code_t ret = KIIE_FAIL;
    struct curl_slist* headers = NULL;

    /* prepare headers */
    headers = prv_common_request_headers(app, access_token, NULL);
    if (headers == NULL) {
        ret = KIIE_LOWMEMORY;
        goto ON_EXIT;
    }

    ret = prv_execute_curl(app->curl_easy, url, GET, NULL, headers,
	    response_status_code, response_body, response_headers, error);

ON_EXIT:
    curl_slist_free_all(headers);

    return ret;
}

kii_error_code_t prv_kii_http_head(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error)
{
    return KIIE_FAIL;
}

kii_error_code_t prv_kii_http_patch(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* opt_etag,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    return KIIE_FAIL;
}

kii_error_code_t prv_kii_http_post(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    return KIIE_FAIL;
}

kii_error_code_t prv_kii_http_put(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* opt_etag,
        kii_bool_t if_none_match,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    return KIIE_FAIL;
}
