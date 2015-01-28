/*
   kii_utils.c
   KiiThingSDK

   Copyright (c) 2014 Kii. All rights reserved.
*/

#include "kii_custom.h"
#include "kii_prv_utils.h"
#include "kii_prv_types.h"

#include <stdarg.h>

static size_t prv_url_encoded_len(const char* element);
static char* prv_url_encoded_copy(char* s1, const char* s2);

char* prv_build_url(const char* first, ...)
{
    size_t size = 0;
    char* retval = NULL;

    if (first == NULL) {
        return NULL;
    }

    /* calculate size. */
    {
        const char* element = NULL;
        va_list list;
        va_start(list, first);
        for (element = first; element != NULL; element = va_arg(list, char*)) {
            /* last "+ 1" means size of '/' or '\0'. */
            size = size + prv_url_encoded_len(element) + 1;
        }
        va_end(list);
    }

    /* alloc size. */
    retval = kii_malloc(size);
    if (retval == NULL) {
        return NULL;
    }

    kii_memset(retval, 0, size);

    /* copy elements. */
    {
        const char* element = NULL;
        va_list list;
        va_start(list, first);

        /* copy first element. */
        prv_url_encoded_copy(retval, first);

        for (element = va_arg(list, char*); element != NULL;
                element = va_arg(list, char*)) {
            size_t len = kii_strlen(retval);
            retval[len] = '/';
            retval[len + 1] = '\0';
            prv_url_encoded_copy(&retval[len + 1], element);
        }
        va_end(list);
    }

    return retval;
}

static size_t prv_url_encoded_len(const char* element)
{
    M_KII_ASSERT(element != NULL);

    /* TODO: calculate url encoded length. */
    return kii_strlen(element);
}

static char* prv_url_encoded_copy(char* s1, const char* s2)
{
    M_KII_ASSERT(s1 != NULL);
    M_KII_ASSERT(s2 != NULL);

    /* TODO: copy url encoded s2 string to s1. */
    return kii_strncpy(s1, s2, kii_strlen(s2)+1);
}

kii_char_t* prv_new_header_string(const kii_char_t* key,
                                  const kii_char_t* value)
{
    size_t keyLen = kii_strlen(key);
    size_t valLen = kii_strlen(value);
    size_t len = keyLen + kii_strlen(":") + valLen;
    kii_char_t* ret = kii_malloc(len + 1);
    
    M_KII_ASSERT(keyLen > 0);
    M_KII_ASSERT(valLen > 0);
    M_KII_ASSERT(ret != NULL);
    
    if (ret == NULL) {
        return NULL;
    }
    
    ret[keyLen] = '\0';
    kii_strncpy(ret, key, keyLen + 1);
    ret[keyLen] = ':';
    ret[keyLen + 1] = '\0';
    kii_strncat(ret, value, valLen + 1);
    return ret;
}

kii_char_t* prv_new_auth_header_string(const kii_char_t* access_token)
{
    const kii_char_t* authbearer = "authorization: bearer ";
    size_t authbearerLen = kii_strlen(authbearer);
    size_t tokenLen = kii_strlen(access_token);
    size_t len = authbearerLen + tokenLen;
    kii_char_t* ret = malloc(len + 1);
    
    M_KII_ASSERT(authbearerLen > 0);
    M_KII_ASSERT(tokenLen > 0);
    M_KII_ASSERT(ret != NULL);
    
    if (ret == NULL) {
        return NULL;
    }
    
    ret[authbearerLen] = '\0';
    kii_strncpy(ret, authbearer, authbearerLen + 1);
    kii_strncat(ret, access_token, tokenLen + 1);
    return ret;
}

int prv_log(const char* format, ...)
{
    int retval = 0;
    va_list list;
    va_start(list, format);
    retval = vprintf(format, list);
    va_end(list);
    printf("\n");
    return retval;
}

int prv_log_no_LF(const char* format, ...)
{
    int retval = 0;
    va_list list;
    va_start(list, format);
    retval = vprintf(format, list);
    va_end(list);
    return retval;
}
