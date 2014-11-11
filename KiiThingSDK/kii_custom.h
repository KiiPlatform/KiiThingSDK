/*
  kii_custom.h
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#ifndef __KiiThingSDK__kii_custom__
#define __KiiThingSDK__kii_custom__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "jansson.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_KII_FREE_NULLIFY(ptr) \
kii_free((ptr));\
(ptr) = NULL;

#define M_KII_ASSERT(exp) \
assert((exp));

typedef int kii_int_t;
typedef unsigned int kii_uint_t;
typedef unsigned long kii_ulong_t;
typedef char kii_char_t;
typedef json_t kii_json_t;

void* kii_malloc(size_t size);
void* kii_memset(void* buf, int ch, size_t n);
void* kii_memcpy(void* buf1, const void* buf2, size_t n);
void kii_free(void* ptr);
char* kii_strdup(const char* s);
char* kii_strncat(char* s1, const char* s2, size_t n);
size_t kii_strlen(const char* str);
char* kii_strncpy(char *s1, const char *s2, size_t n);
void* kii_realloc(void* ptr, size_t size);
int kii_strncmp(const char *s1, const char *s2, size_t n);
int kii_tolower(int c);
/** Dispose kii_char_t allocated by SDK.
 * @param [in] char_ptr kii_char_t instance should be disposed
 */
void kii_dispose_kii_char(kii_char_t* char_ptr);
/** decrease reference of kii_json_t allocated by SDK.
 * @param [in] json json instance.
 */
void kii_json_decref(kii_json_t* json);

#ifdef __cplusplus
}
#endif

#endif /* defined(__KiiThingSDK__kii_custom__) */
