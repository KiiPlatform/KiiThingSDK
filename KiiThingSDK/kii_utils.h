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

  Outputted header_list must be freed by caller of this method with
  curl_slist_free_all.

  If all headers are appended to header_list, then return KII_TRUE,
  otherwise KII_FALSE. even If this method returns KII_FALSE,
  header_list may have some headers. So caller of this method must
  free header_list. */
kii_bool_t prv_curl_slist_append(struct curl_slist** header_list, ...);

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_utils__) */
