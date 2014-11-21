/*
  kii_cloud.h
  KiiThingSDK

  Copyright (c) 2014 Kii. All rights reserved.
*/

#ifndef KiiThingSDK_kii_cloud_h
#define KiiThingSDK_kii_cloud_h

#include "jansson.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma mark for low level interface customization
    
typedef int kii_int_t;
typedef unsigned int kii_uint_t;
typedef unsigned long kii_ulong_t;
typedef char kii_char_t;

#define M_KII_FREE_NULLIFY(ptr) \
kii_free((ptr));\
(ptr) = NULL;

#define M_KII_ASSERT(exp) \
assert((exp));
    

#pragma mark thing sdk interfaces

static const char KII_SITE_JP[] = "https://api-jp.kii.com/api"; /** Site JP */
static const char KII_SITE_US[] = "https://api.kii.com/api"; /** Site US */
static const char KII_SITE_CN[] = "https://api-cn2.kii.com/api"; /** Site CN */
static const char KII_SITE_SG[] = "https://api-sg.kii.com/api"; /** Site SG */

static const char KII_ECODE_CONNECTION[] = "CONNECTION_ERROR";
static const char KII_ECODE_PARSE[] = "PARSE_ERROR";

/** boolean type */
typedef enum kii_bool_t {
    KII_FALSE,
    KII_TRUE
} kii_bool_t;

typedef enum kii_error_code_t {
    KIIE_OK = 0,
    KIIE_FAIL,
    KIIE_LOWMEMORY,
    KIIE_RESPWRITE
} kii_error_code_t;

/** Represents application.
 * should be disposed by kii_dispose_app(kii_app_t)
 */
typedef struct prv_kii_app_t* kii_app_t;

/** Represents bucket.
 * should be disposed by kii_dispose_bucket(kii_bucket_t)
 */
typedef struct prv_kii_bucket_t* kii_bucket_t;
typedef struct prv_kii_topic_t* kii_topic_t;

/** Dispose kii_char_t allocated by SDK.
 * @param [in] char_ptr kii_char_t instance should be disposed
 */
void kii_dispose_kii_char(kii_char_t* char_ptr);

/** Represents thing.
 * should be disposed by kii_dispose_thing(const kii_thing_t).
 * Can be serialized by kii_thing_serialize(const kii_thing_t).
 * Can be deserialized by kii_thing_deserialize(const kii_char_t*)
 * with serialized string obtained by kii_thing_serialize(const kii_thing_t).
 */
typedef struct prv_kii_thing_t* kii_thing_t;

/** Dispose kii_thing_t instance.
 * @param [in] thing kii_thing_t instance should be disposed.
 */
void kii_dispose_thing(kii_thing_t thing);

/** Serialize kii_thing_t instance into string.
 * @param [in] thing kii_thing_t instance should be serialized.
 * After the registration of thing, you will need to store string in 
 * persistent storage to operate thing scope bucket, object and topic, etc.
 * to avoid getting thing information from cloud after the application
 * restart.
 * @return string to be stored. should be freed by application.
 * @see kii_thing_deserialize(const kii_char_t*)
 */
kii_char_t* kii_thing_serialize(const kii_thing_t thing);

/** Deserialize kii_thing_t instance from serialized string.
 * @param [in] serialized_thing string obtaied by
 * kii_thing_serialize(const kii_thing_t)
 * After the registration of thing, you will need to store string in
 * persistent storage to operate thing scope bucket, object and topic, etc.
 * to avoid getting thing information from cloud after the application
 * restart.
 * @return kii_thing_t instance restored from the string.
 * @see kii_thing_serialize(const kii_thing_t)
 */
kii_thing_t kii_thing_deserialize(const kii_char_t* serialized_thing);

/** Represents error.
 */
typedef struct kii_error_t {
    int status_code; /**< HTTP status code */
    kii_char_t error_code[50]; /**< Error code returned from kii cloud */
} kii_error_t;


/** Represents MQTT endpoint.
 * should be disposed by kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t*)
 */
typedef struct kii_mqtt_endpoint_t {
    kii_char_t* username; /**< username for connecting MQTT endpoint. */
    kii_char_t* password; /**< password for connecting MQTT endpoint. */
    kii_char_t* topic; /** topic for subscription. */
    kii_char_t* host; /** hostname of MQTT endpoint. */
    kii_uint_t port_tcp; /** port number of MQTT endpoint for tcp. */
    kii_uint_t port_ssl; /** port number of MQTT endpoint for ssl. */
    /** valid period of this MQTT endpoint in second.
     * You need to get new endpoint information when this
     * period has elapsed.
     */
    kii_ulong_t ttl;
} kii_mqtt_endpoint_t;

/** Set up program environment.
 * This function must be called at least once within a program
 * (a program is all the code that shares a memory space) before the program
 * calls any other function in kii sdk. 
 * The environment it sets up is constant for the life of the program and
 * is the same for every program, so multiple calls have the same effect
 * as one call.
 *
 * This function is not thread safe.
 * You must not call it when any other thread in the program
 * (i.e. a thread sharing the same memory) is running
 * This doesn't just mean no other thread that is using kii sdk.
 * Because kii_global_init() calls functions of other libraries that are
 * similarly thread unsafe,
 * it could conflict with any other thread that uses these other libraries.
 *
 * @retrun if return code is not KIIE_OK, something went wrong and you cannot
 * use other kii sdk functions.
 */
kii_error_code_t kii_global_init(void);

/**
 * This function releases resources acquired by kii_global_init.
 * You should call kii_global_cleanup once
 * for each call you make to kii_global_init, after you are done using kii sdk.
 *
 * This function is not thread safe.
 * You must not call it when any other thread in the program
 * (i.e. a thread sharing the same memory) is running
 * This doesn't just mean no other thread that is using kii sdk.
 * Because kii_global_init() calls functions of other libraries that are
 * similarly thread unsafe,
 * it could conflict with any other thread that uses these other libraries.
 */
void kii_global_cleanup(void);

/** Init application.
 * obtained instance should be disposed by application.
 * @param [in] app_id application id
 * @param [in] app_key application key
 * @param [in] site_url application site .
 * choose from KII_SITE_JP, KII_SITE_US, KII_SITE_CN or KII_SITE_SG
 * @return kii_app_t instance.
 * @see kii_dispose_app(kii_app_t)
 */
kii_app_t kii_init_app(const kii_char_t* app_id,
                       const kii_char_t* app_key,
                       const kii_char_t* site_url);

/** Obtain error detail happens last.
 * @returns error detail.
 */
kii_error_t* kii_get_last_error(kii_app_t app);

/** Dispose kii_app_t instance.
 * @param [in] app kii_app_t instance should be disposed.
 */
void kii_dispose_app(kii_app_t app);

/** Dispose kii_bucket_t instance.
 * @param [in] bucket kii_bucket_t instance should be disposed.
 */
void kii_dispose_bucket(kii_bucket_t bucket);

/** Dispose kii_topic_t instance.
 * @param [in] topic kii_topic_t instance should be disposed.
 */
void kii_dispose_topic(kii_topic_t topic);

/** Dispose kii_mqtt_endpoint_t instance.
 * @param [in] endpoint kii_mqtt_endpoint_t instance should be disposed.
 */
void kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t* endpoint);

/** Register thing to Kii Cloud.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] vendor_thing_id identifier of the thing
 * should be unique in application.
 * @param [in] thing_password thing password for security.
 * @param [in] opt_thing_type type name of the thing. Can be omitted by
 * specifying NULL.
 * @param [in] user_data additional information about the thing.
 * TODO: details should be linked here.
 * @param [out] instance represents registered thing. Should be disposed by
 * kii_dispose_thing(const kii_thing_t*) by application.
 * @param [out] access_token would be used to
 * authorize access to Kii Cloud by the thing. (CRUD object, etc.)
 * NULL if failed to register.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_register_thing(kii_app_t app,
                                    const kii_char_t* vendor_thing_id,
                                    const kii_char_t* thing_password,
                                    const kii_char_t* opt_thing_type,
                                    const json_t* user_data,
                                    kii_thing_t* out_thing,
                                    kii_char_t** out_access_token);

/** Init thing scope bucket.
 * @param [in] registered thing instance
 * @return kii bucket instance. Should be disposed by
 * kii_dispose_bucket(kii_bucket_t)
 */
kii_bucket_t kii_init_thing_bucket(const kii_thing_t thing,
                                   const kii_char_t* bucket_name);

/** Create new object.
 * Bucket is created automatically if have not been created yet.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket specify bucket contains object.
 * @param [in] contents specify key-values of object.
 * @param [out] out_object_id id of the object created by this api.
 * NULL if failed to create.
 * You can pass NULL if you don't need to know id.
 * If NULL passed, no resource is allocated for id.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * You can pass NULL if you don't need to know etag.
 * If NULL passed, no resource is allocated for etag.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_create_new_object(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_bucket_t bucket,
                                       const json_t* contents,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag);

/** Create new object with specified ID.
 * Bucket is created automatically if have not been created yet.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @param [in] contents specify key-values of object.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * You can pass NULL if you don't need to know etag.
 * If NULL passed, no resource is allocated for etag.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_create_new_object_with_id(kii_app_t app,
                                               const kii_char_t* access_token,
                                               const kii_bucket_t bucket,
                                               const kii_char_t* object_id,
                                               const json_t* contents,
                                               kii_char_t** out_etag);

/** Update object with patch.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * Key-Value pair which contained in patch would be update object partially.
 * Other Key-Value pair in existing object would not be updated.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @param [in] patch patch data.
 * @param [in] opt_etag if NULL apply patch regardless of updates on
 * server. if not NULL, check opt_etag value and apply patch only if
 * the etag matches on server.
 * Specify NULL otherwise.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * You can pass NULL if you don't need to know etag.
 * If NULL passed, no resource is allocated for etag.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_patch_object(kii_app_t app,
                                  const kii_char_t* access_token,
                                  const kii_bucket_t bucket,
                                  const kii_char_t* object_id,
                                  const json_t* patch,
                                  const kii_char_t* opt_etag,
                                  kii_char_t** out_etag);

/** Replace object contents with specified object.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * Key-Value pair which contained in the request replaces whole contents of
 * object.
 * Key-Value pair which is not exists in the object would be deleted.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @param [in] replace_contents replacement data.
 * @param [in] opt_etag if NULL apply patch regardless of updates on
 * server. if not NULL, check opt_etag value and apply patch only if
 * the etag matches on server.
 * Specify NULL otherwise.
 * @param [out] out_etag etag of updated object.
 * NULL if failed to update.
 * You can pass NULL if you don't need to know etag.
 * If NULL passed, no resource is allocated for etag.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_replace_object(kii_app_t app,
                                    const kii_char_t* access_token,
                                    const kii_bucket_t bucket,
                                    const kii_char_t* object_id,
                                    const json_t* replace_contents,
                                    const kii_char_t* opt_etag,
                                    kii_char_t** out_etag);

/** Get object contents with specified id.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket specify bucket contains object.
 * @param [in] bucket_name specify name of the bucket.
 * @param [in] object_id specify id of the object.
 * @param [out] out_etag etag of server object.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_get_object(kii_app_t app,
                                const kii_char_t* access_token,
                                const kii_bucket_t bucket,
                                const kii_char_t* object_id,
                                json_t** out_contents,
                                kii_char_t** out_etag);

/** Delete object with specified id.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_delete_object(kii_app_t app,
                                   const kii_char_t* access_token,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id);

/** Subscribe to bucket push notification.
 * After subscribed to the bucket, you can receive push notification when
 * the event happened in this bucket thru MQTT endpoint which can be retrieved
 * by kii_get_mqtt_endpoint().
 * bucket is created automatically if have not been created yet.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket target bucket to subscribe.
 * @return KIIE_OK if succeeded to subscribe or already subscribed.
 * Otherwise failed.
 * you can check details by calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_subscribe_bucket(kii_app_t app,
                                      const kii_char_t* access_token,
                                      const kii_bucket_t bucket);

/** Unsubscribe bucket push notification.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket target bucket to unsubscribe.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_unsubscribe_bucket(kii_app_t app,
                                        const kii_char_t* access_token,
                                        const kii_bucket_t bucket);

/** Check whether the bucket is subscribed or not.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] bucket target bucket to unsubscribe.
 * @param [out] is_subscribed value would not be changed if the execution is
 * failed.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_is_bucket_subscribed(kii_app_t app,
                                          const kii_char_t* access_token,
                                          const kii_bucket_t bucket,
                                          kii_bool_t* out_is_subscribed);


/** Init thing scope topic.
 * @param [in] registered thing instance
 * @return kii topic instance. Should be disposed by
 * kii_dispose_topic(kii_topic_t)
 */
kii_topic_t kii_init_thing_topic(const kii_thing_t thing,
                                   const kii_char_t* topic_name);

/** Create thing scope topic.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] topic target topic to create.
 * @return KIIE_OK if succeeded to create or already created. Otherwise failed.
 * you can check details by calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_create_topic(kii_app_t app,
                                  const kii_char_t* access_token,
                                  const kii_topic_t topic);

/** Subscribe to topic push notification.
 * After subscribed to the topic, you can receive push notification when
 * the message is sent to the topic.
 * You can receive message thru MQTT endpoint which can be retrieved by
 * kii_get_mqtt_endpoint().
 * Topic is created automatically if have not been created yet.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] topic target topic to subscribe.
 * @return KIIE_OK if succeeded to subscribe or already subscribed.
 * Otherwise failed.
 * you can check details by calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_subscribe_topic(kii_app_t app,
                                     const kii_char_t* access_token,
                                     const kii_topic_t topic);

/** Unsubscribe topic push notification.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] topic target topic to unsubscribe.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_unsubscribe_topic(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_topic_t topic);

/** Check whether the topic is subscribed or not.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] topic target topic to unsubscribe.
 * @param [out] is_subscribed value would not be changed if the execution is
 * failed.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_is_topic_subscribed(kii_app_t app,
                                         const kii_char_t* access_token,
                                         const kii_topic_t topic,
                                         kii_bool_t* out_is_subscribed);

/** Install push for the thing.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] development indicate whether this installation is development
 * or production.
 * @param [out] out_installation_id id of installation.
 * used for getting MQTT endpoint by kii_get_MQTT_endpoint().
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_install_thing_push(kii_app_t app,
                                        const kii_char_t* access_token,
                                        kii_bool_t development,
                                        kii_char_t** out_installation_id);


/** Get MQTT endpoint to retrieve message
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] access_token specify access token of authur.
 * @param [in] installation_id obtained by kii_install_thing_push()
 * @param [out] out_endpoint endpoint information.
 * Reference would be null if failed.
 * Should be disposed by kii_dispose_mqtt_endpoint(kii_mqtt_endpoint_t*).
 * @param [out] out_retry_after_in_second Reference would be set when failed to
 * get endpoint due to its not ready.
 * You need to retry after this period elapsed.
 */
kii_error_code_t kii_get_mqtt_endpoint(kii_app_t app,
                                       const kii_char_t* access_token,
                                       const kii_char_t* installation_id,
                                       kii_mqtt_endpoint_t** out_endpoint,
                                       kii_uint_t* out_retry_after_in_second);


#ifdef __cplusplus
}
#endif

#endif
