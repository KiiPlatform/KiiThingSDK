#ifndef KiiThingSDK_kii_prv_http_execute_h
#define KiiThingSDK_kii_prv_http_execute_h

#include "kii_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

kii_error_code_t prv_kii_http_execute(
        const kii_char_t* http_method,
        const kii_char_t* url,
        json_t* request_headers,
        const kii_char_t* request_body,
        kii_int_t* status_code,
        json_t** response_headers,
        kii_char_t** response_body);

#ifdef __cplusplus
}
#endif

#endif/*KiiThingSDK_kii_prv_http_execute_h*/
