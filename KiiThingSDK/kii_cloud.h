//
//  kii_cloud.h
//  KiiThingSDK
//
//  Copyright (c) 2014 Kii. All rights reserved.
//

#ifndef KiiThingSDK_kii_cloud_h
#define KiiThingSDK_kii_cloud_h

#include "jannson/jansson.h"

#define KII_SITE_JP "https://api-jp.kii.com/api" /** Site JP */
#define KII_SITE_US "https://api.kii.com/api" /** Site US */
#define KII_SITE_CN "https://api-cn2.kii.com/api" /** Site CN */
#define KII_SITE_SG "https://api-sg.kii.com/api" /** Site SG */

/** boolean type */
typedef enum kii_bool_t {
    KII_FALSE,
    KII_TRUE
} kii_bool_t;

typedef enum kii_error_code_t {
    KIIE_OK = 0,
    KIIE_FAIL
} kii_error_code_t;

/** Represents application.
 * should be disposed by kii_dispose_app(kii_app_t)
 */
typedef void* kii_app_t;

/** Represents bucket.
 * should be disposed by kii_dispose_bucket(kii_bucket_t)
 */
typedef void* kii_bucket_t;
typedef int kii_int_t;
typedef char kii_char_t;
typedef json_t kii_json_t;

/** Represents error.
 * should be disposed by kii_dispose_error(kii_error_t)
 */
typedef struct kii_error_t {
    int status_code; /**< HTTP status code */
    kii_char_t* error_code; /**< Error code returned from kii cloud */
} kii_error_t;

/** Init application.
 * obtained instance should be disposed by application.
 * @param [in] app_id application id
 * @param [in] app_key application key
 * @param [in] site_url application site .
 * choose from KII_SITE_JP, KII_SITE_US, KII_SITE_CN or KII_SITE_SG
 * @return kii_app_t instance.
 * @see kii_dispose_app(kii_app_t)
 */
kii_app_t kii_init_app(const char* app_id,
                       const char* app_key,
                       const char* site_url);

/** Obtain error detail happens last.
 * @return error detail. should be disposed by kii_dispose_error(kii_error_t*)
 */
kii_error_t kii_get_last_error(kii_app_t app);

/** Dispose kii_app_t instance.
 * @param [in] app kii_app_t instance should be disposed.
 */
void kii_dispose_app(kii_app_t app);

/** Dispose kii_error_t instance.
 * @param [in] error kii_error_t instance should be disposed.
 */
void kii_dispose_error(kii_error_t* error);

/** Dispose kii_bucket_t instance.
 * @param [in] bucket kii_bucket_t instance should be disposed.
 */
void kii_dispose_bucket(kii_bucket_t bucket);

/** Dispose kii_char_t allocated by SDK.
 * @param [in] char_ptr kii_char_t instance should be disposed
 */
void kii_dispose_kii_char(kii_char_t* char_ptr);

/** Dispose kii_json_t allocated by SDK.
 * @param [in] json json instance should be disposed
 */
void kii_dispose_kii_json(kii_json_t* json);

/** Register thing to Kii Cloud.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] thing_vendor_id identifier of the thing
 * should be unique in application.
 * @param [in] thing_password thing password for security.
 * @param [in] user_data additional information about the thing.
 * TODO: details should be linked here.
 * @param [out] access_token would be used to
 * authorize access to Kii Cloud by the thing. (CRUD object, etc.)
 * NULL if failed to register.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_register_thing(const kii_app_t app,
                                    const kii_char_t* thing_vendor_id,
                                    const kii_char_t* thing_password,
                                    const kii_json_t* user_data,
                                    kii_char_t** out_access_token);

/** Init thing bucket bucket.
 * @param [in] thing_vendor_id identifier of the thing
 * @return kii bucket instance. Should be disposed by
 * kii_dispose_bucket(kii_bucket_t)
 */
kii_bucket_t kii_init_thing_bucket(kii_char_t* thing_vendor_id,
                                   kii_char_t* bucket_name);

/** Create new object.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] bucket specify bucket contains object.
 * @param [in] contents specify key-values of object.
 * @param [in] access_token specify access token of authur.
 * @param [out] out_object_id id of the object created by this api.
 * NULL if failed to create.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_create_new_object(const kii_app_t app,
                                       const kii_bucket_t bucket,
                                       const kii_json_t* contents,
                                       const kii_char_t* access_token,
                                       kii_char_t** out_object_id,
                                       kii_char_t** out_etag);

/** Create new object with specified ID.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @param [in] contents specify key-values of object.
 * @param [in] access_token specify access token of authur.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_create_new_object_with_id(const kii_app_t app,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id,
                                   const kii_json_t* contents,
                                   const kii_char_t* access_token,
                                   kii_char_t** out_etag);

/** Update object with patch.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * Key-Value pair which contained in patch would be update object partially.
 * Other Key-Value pair in existing object would not be updated.
 * @param [in] app kii application uses this thing.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @param [in] patch patch data.
 * @param [in] force_update if kii_true, apply patch regardless of updates on
 * server. If kii_false check opt_etag value and applya patch only if the etag
 * matches.
 * @param [in] opt_etag required if force_update is kii_true.
 * Specify NULL otherwise.
 * @param [in] access_token specify access token of authur.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_patch_object(const kii_app_t app,
                                  const kii_bucket_t bucket,
                                  const kii_char_t* object_id,
                                  const kii_json_t* patch,
                                  const kii_bool_t force_update,
                                  const kii_char_t* opt_etag,
                                  const kii_char_t* access_token,
                                  kii_char_t** out_etag);

/** Replace object contents with specified object.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * Key-Value pair which contained in the request replaces whole contents of
 * object.
 * Key-Value pair which is not exists in the object would be deleted.
 * @param [in] app kii application uses this thing.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @param [in] replace_contents replacement data.
 * @param [in] force_update if kii_true, apply patch regardless of updates on
 * server. If kii_false check opt_etag value and applya patch only if the etag
 * matches.
 * @param [in] opt_etag required if force_update is kii_true.
 * Specify NULL otherwise.
 * @param [in] access_token specify access token of authur.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_replace_object(const kii_app_t app,
                                    const kii_bucket_t bucket,
                                    const kii_char_t* object_id,
                                    const kii_json_t* replace_contents,
                                    const kii_bool_t force_update,
                                    const kii_char_t* opt_etag,
                                    const kii_char_t* access_token,
                                    kii_char_t** out_etag);

/** Get object contents with specified id.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] bucket specify bucket contains object.
 * @param [in] bucket_name specify name of the bucket.
 * @param [in] object_id specify id of the object.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_get_object(const kii_app_t app,
                                const kii_bucket_t bucket,
                                const kii_char_t* object_id,
                                const kii_json_t** out_contents);

/** Delete object with specified id.
 * This api performes the entire request in a blocking manner
 * and returns when done, or if it failed.
 * @param [in] app kii application uses this thing.
 * @param [in] bucket specify bucket contains object.
 * @param [in] object_id specify id of the object.
 * @return KIIE_OK if succeeded. Otherwise failed. you can check details by
 * calling kii_get_last_error(kii_app_t).
 */
kii_error_code_t kii_delete_object(const kii_app_t app,
                                   const kii_bucket_t bucket,
                                   const kii_char_t* object_id);


#endif