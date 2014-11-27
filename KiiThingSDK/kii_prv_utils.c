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

struct curl_slist*
prv_common_request_headers(
                           const kii_app_t app,
                           const kii_char_t* opt_access_token,
                           const kii_char_t* opt_content_type)
{
    kii_char_t* app_id_hdr = NULL;
    kii_char_t* app_key_hdr = NULL;
    struct curl_slist* retval = NULL;
    
    M_KII_ASSERT(app->app_id != NULL);
    M_KII_ASSERT(app->app_key != NULL);
    
    app_id_hdr = prv_new_header_string("x-kii-appid", app->app_id);
    app_key_hdr = prv_new_header_string("x-kii-appkey", app->app_key);
    if (app_id_hdr == NULL || app_key_hdr == NULL) {
        retval = NULL;
        goto ON_EXIT;
    }
    
    retval = prv_curl_slist_create(app_id_hdr, app_key_hdr, NULL);
    if (retval == NULL) {
        goto ON_EXIT;
    }
    
    if (opt_access_token != NULL) {
        kii_char_t* auth_hdr = prv_new_auth_header_string(opt_access_token);
        struct curl_slist* tmp = (auth_hdr == NULL) ? NULL :
        curl_slist_append(retval, auth_hdr);
        
        kii_dispose_kii_char(auth_hdr);
        if (tmp == NULL) {
            curl_slist_free_all(retval);
            retval = NULL;
            goto ON_EXIT;
        }
        retval = tmp;
    }
    
    if (opt_content_type != NULL) {
        kii_char_t* content_type_hdr = prv_new_header_string("content-type",
                                                             opt_content_type);
        struct curl_slist* tmp = (content_type_hdr == NULL) ? NULL :
        curl_slist_append(retval, content_type_hdr);
        
        kii_dispose_kii_char(content_type_hdr);
        if (tmp == NULL) {
            curl_slist_free_all(retval);
            retval = NULL;
            goto ON_EXIT;
        }
        retval = tmp;
    }
    
ON_EXIT:
    M_KII_FREE_NULLIFY(app_id_hdr);
    M_KII_FREE_NULLIFY(app_key_hdr);
    
    return retval;
}

struct curl_slist*
prv_curl_slist_append_key_and_value(struct curl_slist *headers,
                                    const kii_char_t *key,
                                    const kii_char_t *value)
{
    kii_char_t* hdr = prv_new_header_string(key, value);
    struct curl_slist* retval = (hdr == NULL) ? NULL :
    curl_slist_append(headers, hdr);
    kii_dispose_kii_char(hdr);
    return retval;
}

void prv_log_req_heder(struct curl_slist* header)
{
    while (header != NULL) {
        prv_log("req header: %s", header->data);
        header = header->next;
    }
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
