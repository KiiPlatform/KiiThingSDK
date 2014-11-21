/*
  kii_utils.h
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#ifndef __KiiThingSDK__kii_utils__
#define __KiiThingSDK__kii_utils__

#include <stdio.h>

#include "curl.h"
#include "kii_custom.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define M_KII_DEBUG(f) f
#else
#define M_KII_DEBUG(f)
#endif

/* The last argument of this method must be NULL. */
/* Returned value of this method must be freed by caller of this method. */
char* prv_build_url(const char* first, ...);

/* The last argument of this method must be NULL.

   If all headers are appended, then this method returns struct
   curl_slist ponter. Otherwise returns NULL.

   Returned value must be freed by caller of this method with
   curl_slist_free_all. */
struct curl_slist* prv_curl_slist_create(const char* first, ...);

kii_char_t* prv_new_header_string(const kii_char_t* key,
                                  const kii_char_t* value);

kii_char_t* prv_new_auth_header_string(const kii_char_t* access_token);

struct curl_slist*
prv_common_request_headers(
                           const kii_app_t app,
                           const kii_char_t* opt_access_token,
                           const kii_char_t* opt_content_type);

struct curl_slist*
prv_curl_slist_append_key_and_value(struct curl_slist* headers,
                                    const kii_char_t* key,
                                    const kii_char_t* value);

void prv_log_req_heder(struct curl_slist* header);

int prv_log(const char* format, ...);
int prv_log_no_LF(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_utils__) */
