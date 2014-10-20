//
//  kii_logger.h
//  KiiThingSDK
//
//  Created by 熊野 聡 on 2014/10/20.
//  Copyright (c) 2014年 Kii. All rights reserved.
//

#ifndef __KiiThingSDK__kii_logger__
#define __KiiThingSDK__kii_logger__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define M_KII_LOG(f_, ...) printf((f_), __VA_ARGS__)
#else
#define M_KII_LOG(f_, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_logger__) */
