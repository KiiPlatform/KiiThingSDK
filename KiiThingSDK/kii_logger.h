/*
  kii_logger.h
  KiiThingSDK

  Created by 熊野 聡 on 2014/10/20.
  Copyright (c) 2014年 Kii. All rights reserved.
*/

#ifndef __KiiThingSDK__kii_logger__
#define __KiiThingSDK__kii_logger__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define M_KII_DEBUG(f) f
#else
#define M_KII_DEBUG(f)
#endif

#ifdef __cplusplus
}
#endif

int prv_log(const char* format, ...);
int prv_log_no_LF(const char* format, ...);

#endif /* defined(__KiiThingSDK__kii_logger__) */
