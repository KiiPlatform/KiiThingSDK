/*
  kii_libc.c
  KiiThingSDK

  Created by 熊野 聡 on 2014/10/20.
  Copyright (c) 2014年 Kii. All rights reserved.
*/

#include "kii_libc.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void* kii_malloc(size_t size)
{
    return malloc(size);
}

void* kii_memset(void* buf, int ch, size_t n) {
    return memset(buf, ch, n);
}

void* kii_memcpy(void* buf1, const void* buf2, size_t n)
{
  return memcpy(buf1, buf2, n);
}

void kii_free(void* ptr)
{
    free(ptr);
}

char* kii_strdup(const char* s)
{
    return strdup(s);
}

int kii_sprintf(char* dest, const char* format, ...)
{
    int ret = 0;
    va_list list;
    va_start(list, format);
    ret = vsprintf(dest, format, list);
    va_end(list);
    return ret;
}

char* kii_strcat(char* s1, const char* s2)
{
    return strcat(s1, s2);
}

char* kii_strncat(char* s1, const char* s2, size_t n)
{
    return strncat(s1, s2, n);
}

size_t kii_strlen(const char* str)
{
    return strlen(str);
}

char* kii_strcpy(char* s1, const char* s2)
{
  return strcpy(s1, s2);
}

void* kii_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

int kii_strncmp(const char *s1, const char *s2, size_t n)
{
    return strncmp(s1, s2, n);
}

int kii_tolower(int c)
{
    return tolower(c);
}
