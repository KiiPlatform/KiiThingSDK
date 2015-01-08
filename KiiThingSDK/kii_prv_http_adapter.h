#ifndef KiiThingSDK_kii_http_adapter_h
#define KiiThingSDK_kii_http_adapter_h

#include "kii_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

kii_error_code_t prv_kii_http_delete(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        json_t* request_headers,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error);

kii_error_code_t prv_kii_http_get(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        json_t* request_headers,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error);

kii_error_code_t prv_kii_http_head(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        json_t* request_headers,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error);

kii_error_code_t prv_kii_http_patch(
        const kii_char_t* url,
        const kii_app_t app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        json_t* request_headers,
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
        json_t* request_headers,
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
        json_t* request_headers,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error);

#ifdef __cplusplus
}
#endif

#endif//KiiThingSDK_kii_http_adapter_h
