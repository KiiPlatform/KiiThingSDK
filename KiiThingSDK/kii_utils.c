/*
   kii_utils.c
   KiiThingSDK

   Copyright (c) 2014 Kii. All rights reserved.
*/

#include "kii_utils.h"

#include "kii_libc.h"

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
    return kii_strcpy(s1, s2);
}

struct curl_slist* prv_curl_slist_create(const char* first, ...)
{
    struct curl_slist* retval = NULL;
    const char* header = NULL;
    va_list list;
    va_start(list, first);
    for (header = first; header != NULL; header = va_arg(list, char*)) {
        struct curl_slist* tmp = curl_slist_append(retval, header);
        if (tmp == NULL) {
            curl_slist_free_all(retval);
            retval = NULL;
            break;
        }
        retval = tmp;
    }
    va_end(list);
    return retval;
}
