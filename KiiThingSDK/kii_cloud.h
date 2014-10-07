//
//  kii_cloud.h
//  KiiThingSDK
//
//  Copyright (c) 2014年 Kii. All rights reserved.
//

#ifndef KiiThingSDK_kii_cloud_h
#define KiiThingSDK_kii_cloud_h

#include "jannson.h"

/** Represents Kii site */
enum kii_site {
    kii_site_jp,
    kii_site_us,
    kii_site_cn,
    kii_site_sg
} kii_site_t;

/** boolean type */
enum kii_bool {
    kii_false,
    kii_true
} kii_bool_t;

/** Represents application.
 * should be disposed by kii_dispose_app(kii_app_t*)
 */
typedef void* kii_app_t;

/** Represents scope.
 * should be disposed by kii_dispose_scope(kii_scope_t*)
 */
typedef void* kii_scope_t;
typedef int kii_int_t;
typedef char kii_char_t;
typedef json_t kii_json_t;

/** Represents error.
 * should be disposed by kii_dispose_error(kii_error_t*)
 */
struct kii_error {
    int status_code; /**< HTTP status code */
    kii_char_t* error_code; /**< Error code returned from kii cloud */
} kii_error_t;

/** Init application.
 * obtained instance should be disposed by application.
 * @param [in] app_id application id
 * @param [in] app_key application key
 * @param [in] site application site
 * @return kii_app_t instance.
 * @see kii_dispose_app(kii_app_t*)
 */
kii_app_t* kii_init_app(const char* app_id,
                        const char* app_key,
                        kii_site_t site);

// Object disposers.
/** Dispose kii_app_t instance.
 * @param [in] app kii_app_t instance should be disposed.
 */
void kii_dispose_app(kii_app_t* app);

/** Dispose kii_error_t instance.
 * @param [in] error kii_error_t instance should be disposed.
 */
void kii_dispose_error(kii_error_t* error);

/** Dispose kii_scope_t instance.
 * @param [in] scope kii_scope_t instance should be disposed.
 */
void kii_dispose_scope(kii_scope_t* scope);

/** Register thing to Kii Cloud.
 * @param [in] app kii application uses this thing.
 * @param [in] thing_vendor_id identifier of the thing
 * should be unique in application.
 * @param [in] thing_password thing password for security.
 * @param [in] user_data additional information about the thing.
 * TODO: details should be linked here.
 * @param [out] access_token would be used to
 * authorize access to Kii Cloud by the thing. (CRUD object, etc.)
 * NULL if failed to register.
 * @param [out] error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_register_thing(const kii_app_t* app,
                        const kii_char_t* thing_vendor_id,
                        const kii_char_t* thing_password,
                        const kii_json_t* user_data,
                        kii_char_t** out_access_token,
                        kii_error_t** out_error);
/** Init thing scope.
 * @param [in] thing_vendor_id identifier of the thing
 * @return kii scope instance. Should be disposed by
 * kii_dispose_scope(kii_scope_t*)
 */
kii_scope_t* kii_init_thing_scope(kii_char_t* thing_vendor_id);

/** Create new object.
 * @param [in] app kii application uses this thing.
 * @param [in] scope specify scope of bucket.
 * @param [in] bucket_name specify name of the bucket.
 * @param [in] contents specify key-values of object.
 * @param [in] access_token specify access token of authur.
 * @param [out] out_object_id id of the object created by this api.
 * NULL if failed to create.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * @param [out] out_error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_create_new_object(const kii_app_t* app,
                           const kii_scope_t* scope,
                           const kii_char_t* bucket_name,
                           const kii_json_t* contents,
                           const kii_char_t* access_token,
                           kii_char_t** out_object_id,
                           kii_char_t** out_etag,
                           kii_error_t** out_error);

/** Create new object with specified ID.
 * @param [in] app kii application uses this thing.
 * @param [in] scope specify scope of bucket.
 * @param [in] bucket_name specify name of the bucket.
 * @param [in] object_id specify id of the object.
 * @param [in] contents specify key-values of object.
 * @param [in] access_token specify access token of authur.
 * @param [out] out_etag etag of created object.
 * NULL if failed to create.
 * @param [out] out_error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_create_new_object_with_id(const kii_app_t* app,
                                   const kii_scope_t* scope,
                                   const kii_char_t* bucket_name,
                                   const kii_char_t* object_id,
                                   const kii_json_t* contents,
                                   const kii_char_t* access_token,
                                   kii_char_t** out_etag,
                                   kii_error_t** out_error);

/** Update object with patch.
 * Key-Value pair which contained in patch would be update object partially.
 * Other Key-Value pair in existing object would not be updated.
 * @param [in] app kii application uses this thing.
 * @param [in] scope specify scope of bucket.
 * @param [in] bucket_name specify name of the bucket.
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
 * @param [out] out_error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_patch_object(const kii_app_t* app,
                      const kii_scope_t* scope,
                      const kii_chart_t* bucket_name,
                      const kii_char_t* object_id,
                      const kii_json_t* patch,
                      const kii_bool_t force_update,
                      const kii_char_t* opt_etag,
                      const kii_char_t* access_token,
                      kii_char_t** out_etag,
                      kii_error_t** out_error);

/** Replace object contents with specified object.
 * Key-Value pair which contained in the request replaces whole contents of
 * object.
 * Key-Value pair which is not exists in the object would be deleted.
 * @param [in] app kii application uses this thing.
 * @param [in] scope specify scope of bucket.
 * @param [in] bucket_name specify name of the bucket.
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
 * @param [out] out_error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_replace_object(const kii_app_t* app
                        const kii_scope_t* scope,
                        const kii_chart_t* bucket_name,
                        const kii_char_t* object_id,
                        const kii_json_t* replace_contents,
                        const kii_bool_t force_update,
                        const kii_char_t* opt_etag,
                        const kii_char_t* access_token,
                        kii_char_t** out_etag,
                        kii_error_t** out_error);

/** Get object contents with specified id.
 * @param [in] app kii application uses this thing.
 * @param [in] scope specify scope of bucket.
 * @param [in] bucket_name specify name of the bucket.
 * @param [in] object_id specify id of the object.
 * @param [out] out_error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_get_object(const kii_app_t* app
                    const kii_scope_t* scope,
                    const kii_chart_t* bucket_name,
                    const kii_char_t* object_id,
                    const kii_json_t** out_contents,
                    kii_error_t** out_error);

/** Delete object with specified id.
 * @param [in] app kii application uses this thing.
 * @param [in] scope specify scope of bucket.
 * @param [in] bucket_name specify name of the bucket.
 * @param [in] object_id specify id of the object.
 * @param [out] out_error represents error if not succeeded.
 * NULL if succeeded.
 */
void kii_delete_object(const kii_app_t* app
                       const kii_scope_t* scope,
                       const kii_chart_t* bucket_name,
                       const kii_char_t* object_id,
                       kii_error_t** out_error);


#endif