/*
  kii_logger.c
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#include "kii_logger.h"

#include <stdarg.h>

int prv_log(const char* format, ...)
{
    int retval = 0;
    va_list list;
    va_start(list, format);
    retval = vprintf(format, list);
    printf("\n");
    va_end(list);
    return retval;
}