/*
  kii_utils.h
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#ifndef __KiiThingSDK__kii_utils__
#define __KiiThingSDK__kii_utils__

#include <stdio.h>

#include "curl.h"
#include "kii_cloud.h"

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_utils__) */
