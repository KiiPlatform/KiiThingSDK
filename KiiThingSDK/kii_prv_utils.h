/*
  kii_utils.h
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#ifndef __KiiThingSDK__kii_utils__
#define __KiiThingSDK__kii_utils__

#include <stdio.h>

#include "kii_custom.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define M_KII_DEBUG(f) f
#else
#define M_KII_DEBUG(f)
#endif

void prv_kii_set_info_in_error(
        kii_error_t* error,
        int status_code,
        const kii_char_t* error_code);

/* The last argument of this method must be NULL. */
/* Returned value of this method must be freed by caller of this method. */
char* prv_build_url(const char* first, ...);

kii_char_t* prv_new_header_string(const kii_char_t* key,
                                  const kii_char_t* value);

kii_char_t* prv_new_auth_header_string(const kii_char_t* access_token);

int prv_log(const char* format, ...);
int prv_log_no_LF(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_utils__) */
