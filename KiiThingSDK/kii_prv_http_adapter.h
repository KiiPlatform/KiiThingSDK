#ifndef KiiThingSDK_kii_http_adapter_h
#define KiiThingSDK_kii_http_adapter_h

#include "kii_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	kii_error_t* error);

kii_error_code_t prv_kii_http_delete(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error);

kii_error_code_t prv_kii_http_get(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error);

kii_error_code_t prv_kii_http_head(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error);

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
        kii_error_t* error);

kii_error_code_t prv_kii_http_post(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error);

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
        kii_error_t* error);

#ifdef __cplusplus
}
#endif

#endif//KiiThingSDK_kii_http_adapter_h