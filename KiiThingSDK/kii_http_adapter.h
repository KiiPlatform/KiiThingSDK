#ifndef KiiThingSDK_kii_http_adapter_h
#define KiiThingSDK_kii_http_adapter_h

#include "kii_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

kii_bool_t kii_http_execute(
        const kii_char_t* http_method,
        const kii_char_t* url,
        json_t* request_headers,
        const kii_char_t* request_body,
        kii_int_t* status_code,
        json_t** response_headers,
        kii_char_t** response_body,
	kii_int_t* adapter_error_code);

#ifdef __cplusplus
}
#endif

#endif/*KiiThingSDK_kii_http_adapter_h*/
