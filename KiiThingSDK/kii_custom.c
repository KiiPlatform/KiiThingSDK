/*
  kii_custom.c
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#include "kii_custom.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

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

kii_char_t* kii_strdup(const kii_char_t* s)
{
    return strdup(s);
}

kii_char_t* kii_strncat(kii_char_t* s1, const kii_char_t* s2, size_t n)
{
    return strncat(s1, s2, n);
}

size_t kii_strlen(const kii_char_t* str)
{
    return strlen(str);
}

kii_char_t* kii_strncpy(kii_char_t *s1, const kii_char_t *s2, size_t n)
{
    return strncpy(s1, s2, n);
}

void* kii_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

int kii_strncmp(const kii_char_t *s1, const kii_char_t *s2, size_t n)
{
    return strncmp(s1, s2, n);
}

int kii_tolower(int c)
{
    return tolower(c);
}
