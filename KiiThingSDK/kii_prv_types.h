/*
  kii_types.h
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#ifndef KiiThingSDK_kii_types_h
#define KiiThingSDK_kii_types_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct prv_kii_app_t {
    kii_char_t* app_id;
    kii_char_t* app_key;
    kii_char_t* site_url;
    CURL* curl_easy;
    kii_error_code_t last_result;
    kii_error_t last_error;
} prv_kii_app_t;

typedef struct prv_kii_thing_t {
    kii_char_t* kii_thing_id; /* thing id assigned by kii cloud */
} prv_kii_thing_t;

typedef struct prv_kii_bucket_t {
    kii_char_t* kii_thing_id;
    kii_char_t* bucket_name;
} prv_kii_bucket_t;

typedef struct prv_kii_topic_t {
    kii_char_t* kii_thing_id;
    kii_char_t* topic_name;
} prv_kii_topic_t;

#ifdef __cplusplus
}
#endif

#endif
