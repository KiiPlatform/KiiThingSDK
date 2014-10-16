//
//  kii_cloud.c
//  KiiThingSDK
//
//  Copyright (c) 2014 Kii. All rights reserved.
//

#include <stdio.h>
#include "kii_cloud.h"
#include "curl.h"


kii_error_code_t kii_global_init(void)
{
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
    return r == CURLE_OK ? KIIE_OK : KIIE_FAIL;
}

void kii_global_clenaup(void)
{
    curl_global_cleanup();
}

kii_app_t kii_init_app(const char* app_id,
                       const char* app_key,
                       const char* site_url)
{
    // TODO: implement it.
    return NULL;
}

kii_error_t* kii_get_last_error(kii_app_t app)
{
    // TODO: implement it.
    return NULL;
}

void kii_dispose_app(kii_app_t app)
{
    // TODO: implement it.
}

void kii_dispose_bucket(kii_bucket_t bucket)
{
    // TODO: implement it.
}

void kii_dispose_topic(kii_topic_t topic)
{
    // TODO: implement it.
}

void kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t* endpoint)
{
    // TODO: implement it.
}

void kii_dispose_kii_char(kii_char_t* char_ptr)
{
    // TODO: implement it.
}

void kii_json_decref(kii_json_t* json)
{
    // TODO: implement it.
}

kii_error_code_t kii_register_thing(kii_app_t app,
                                    const kii_char_t* thing_vendor_id,
                                    const kii_char_t* thing_password,
                                    const kii_json_t* user_data,
                                    kii_char_t** out_access_token)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_bucket_t kii_init_thing_bucket(const kii_char_t* thing_vendor_id,
                                   const kii_char_t* bucket_name)
{
    // TODO: implement it.
    return NULL;
}

kii_error_code_t kii_create_new_object(kii_app_t app,
                                       const kii_bucket_t bucket,
                                       const kii_json_t* contents,
                                       const kii_char_t* access_token,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_create_new_object_with_id(kii_app_t app,
                                               const kii_bucket_t bucket,
                                               const kii_char_t* object_id,
                                               const kii_json_t* contents,
                                               const kii_char_t* access_token,
                                               kii_char_t** out_etag)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_patch_object(kii_app_t app,
                                  const kii_bucket_t bucket,
                                  const kii_char_t* object_id,
                                  const kii_json_t* patch,
                                  const kii_bool_t force_update,
                                  const kii_char_t* opt_etag,
                                  const kii_char_t* access_token,
                                  kii_char_t** out_etag)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_replace_object(kii_app_t app,
                                    const kii_bucket_t bucket,
                                    const kii_char_t* object_id,
                                    const kii_json_t* replace_contents,
                                    const kii_bool_t force_update,
                                    const kii_char_t* opt_etag,
                                    const kii_char_t* access_token,
                                    kii_char_t** out_etag)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_get_object(kii_app_t app,
                                const kii_bucket_t bucket,
                                const kii_char_t* object_id,
                                const kii_char_t* access_token,
                                const kii_json_t** out_contents)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_delete_object(kii_app_t app,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id,
                                   const kii_char_t* access_token)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_subscribe_bucket(const kii_bucket_t bucket,
                                      const kii_char_t* access_token)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_unsubscribe_bucket(const kii_bucket_t bucket,
                                        const kii_char_t* access_token)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_is_bucket_subscribed(const kii_bucket_t bucket,
                                          const kii_char_t* access_token,
                                          kii_bool_t* out_is_subscribed)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_topic_t kii_init_thing_topic(const kii_char_t* thing_vendor_id,
                                 const kii_char_t* topic_name)
{
    // TODO: implement it.
    return NULL;
}

kii_error_code_t kii_subscribe_topic(const kii_topic_t topic,
                                     const kii_char_t* access_token)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_unsubscribe_topic(const kii_topic_t topic,
                                       const kii_char_t* access_token)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_is_topic_subscribed(const kii_topic_t topic,
                                         const kii_char_t* access_token,
                                         kii_bool_t* out_is_subscribed)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_install_thing_push(kii_app_t app,
                                        const kii_char_t* access_token,
                                        kii_char_t** out_installation_id)
{
    // TODO: implement it.
    return KIIE_FAIL;
}

kii_error_code_t kii_get_mqtt_endpoint(kii_app_t app,
                                       const kii_char_t* installation_id,
                                       const kii_char_t* access_token,
                                       kii_mqtt_endpoint_t** out_endpoint,
                                       kii_uint_t* out_retry_after_in_second)
{
    // TODO: implement it.
    return KIIE_FAIL;
}
