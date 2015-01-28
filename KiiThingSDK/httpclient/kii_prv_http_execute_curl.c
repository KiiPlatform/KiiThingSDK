#include "../kii_http_adapter.h"
#include "../kii_custom.h"
#include "../kii_prv_utils.h"
#include "../kii_prv_types.h"

#ifdef XCODE
#include "curl.h"
#else
#include <curl/curl.h>
#endif

typedef enum adapter_error_code_t {
    AEC_OK = 0,
    AEC_FAIL,
    AEC_CURL,
    AEC_LOWMEMORY
} adapter_error_code_t;

static adapter_error_code_t adapter_error_code;
static CURLcode curl_error_code;

static void prv_log_req_heder(struct curl_slist* header)
{
    while (header != NULL) {
        prv_log("req header: %s", header->data);
        header = header->next;
    }
}

static struct curl_slist* convert_request_headers(
        json_t* request_headers)
{
    struct curl_slist* ret = NULL;
    const char* key = NULL;
    json_t* value = NULL;

    json_object_foreach(request_headers, key, value) {
        struct curl_slist* tmp = NULL;
        kii_char_t* hdr = prv_new_header_string(key, json_string_value(value));
        if (hdr == NULL) {
            curl_slist_free_all(ret);
            return NULL;
        }
        tmp = curl_slist_append(ret, hdr);
        kii_dispose_kii_char(hdr);
        if (tmp == NULL) {
            curl_slist_free_all(ret);
            return NULL;
        } else {
            ret = tmp;
        }
    }

    return ret;
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

static adapter_error_code_t prv_execute_curl(CURL* curl,
        const kii_char_t* url,
        prv_kii_req_method_t method,
        const kii_char_t* request_body,
        struct curl_slist* request_headers,
        long* response_status_code,
        kii_char_t** response_body,
        json_t** response_headers)
{
    M_KII_ASSERT(curl != NULL);
    M_KII_ASSERT(url != NULL);
    M_KII_ASSERT(request_headers != NULL);
    M_KII_ASSERT(response_status_code != NULL);

    M_KII_DEBUG(prv_log("request url: %s", url));
    M_KII_DEBUG(prv_log("request method: %d", method));
    M_KII_DEBUG(prv_log("request body: %s", request_body));

    curl_error_code = CURLE_COULDNT_CONNECT; /* set error code as default. */

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
                return AEC_LOWMEMORY;
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
            return AEC_FAIL;
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

    curl_error_code = curl_easy_perform(curl);
    switch (curl_error_code) {
        case CURLE_OK:
            M_KII_DEBUG(prv_log("response: %s", *response_body));
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                    response_status_code);
            return AEC_OK;
        default:
            return AEC_CURL;
    }
}

kii_bool_t kii_http_init(void)
{
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    return ((r == CURLE_OK) ? KII_TRUE : KII_FALSE);
}

void kii_http_cleanup(void)
{
    curl_global_cleanup();
}

kii_bool_t kii_http_execute(
        const kii_char_t* http_method,
        const kii_char_t* url,
        json_t* request_headers,
        const kii_char_t* request_body,
        kii_int_t* status_code,
        json_t** response_headers,
        kii_char_t** response_body)
{
    adapter_error_code_t ret = AEC_FAIL;
    prv_kii_req_method_t method;
    struct curl_slist* headers = NULL;
    long http_status = 0;
    CURL* curl = NULL;

    if (kii_strncmp(http_method, "DELETE", kii_strlen("DELETE")) == 0) {
        method = DELETE;
    } else if (kii_strncmp(http_method, "GET", kii_strlen("GET")) == 0) {
        method = GET;
    } else if (kii_strncmp(http_method, "HEAD", kii_strlen("HEAD")) == 0) {
        method = HEAD;
    } else if (kii_strncmp(http_method, "PATCH", kii_strlen("PATCH")) == 0) {
        method = PATCH;
    } else if (kii_strncmp(http_method, "POST", kii_strlen("POST")) == 0) {
        method = POST;
    } else if (kii_strncmp(http_method, "PUT", kii_strlen("PUT")) == 0) {
        method = PUT;
    } else {
        ret = AEC_FAIL;
        goto ON_EXIT;
    }

    headers = convert_request_headers(request_headers);
    if (headers == NULL) {
        ret = AEC_LOWMEMORY;
        goto ON_EXIT;
    }

    curl = curl_easy_init();

    ret = prv_execute_curl(curl, url, method, request_body, headers,
            &http_status, response_body, response_headers);
    *status_code = (kii_int_t)http_status;

ON_EXIT:
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    adapter_error_code = ret;
    return (ret == AEC_OK) ? KII_TRUE : KII_FALSE;
}

