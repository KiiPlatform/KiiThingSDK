//
//  kii_utils.h
//  KiiThingSDK
//
//  Copyright (c) 2014 Kii. All rights reserved.
//

#ifndef __KiiThingSDK__kii_utils__
#define __KiiThingSDK__kii_utils__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// The last argument of this method must be NULL.
// Returned value of this method must be freed by caller of this method.
char* prv_build_url(const char* first, ...);

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_utils__) */
