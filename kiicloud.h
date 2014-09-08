#ifndef KIICLOUD_INCLUDED
#define KIICLOUD_INCLUDED

/* JSON library: http://www.digip.org/jansson/ */
#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 * Version information
 */

#define KIICLOUD_MAJOR_VERSION  0
#define KIICLOUD_MINOR_VERSION  1
#define KIICLOUD_MICRO_VERSION  0

#define KIICLOUD_VERSION    #KIICLOUD_MAJOR_VERSION "." #KIICLOUD_MICRO_VERSION
#define KIICLOUD_VERSION_HEX    ( \
        (KIICLOUD_MAJOR_VERSION << 16) | \
        (KIICLOUD_MINOR_VERSION <<  8) | \
        (KIICLOUD_MICRO_VERSION <<  0))

/***************************************************************************
 * Types abstraction
 */

typedef int     kii_err_t;
typedef char    kii_char_t;
typedef json_t  kii_json_t;
typedef int     kii_scope_t;
typedef void *  kii_app_t;

/***************************************************************************
 * Application
 */

/**
 * Open an app.
 */
kii_app_t* kii_app_open(void);

/**
 * Initiate an app.
 *
 * \param app Pointer to app.
 * \param endpoint KiiCloud end point. ex. http://api.kii.com/api
 * \param app_id App ID.
 * \param app_key App key.
 */
kii_err_t kii_app_init(
        kii_app_t *app,
        const kii_char_t    *endpoint,
        const kii_char_t    *app_id,
        const kii_char_t    *app_key);

/**
 * Load (deserialize) app info.
 */
kii_err_t kii_app_load(
        kii_app_t           *app,
        const kii_char_t    *data_ptr,
        int                 data_len);

/**
 * Save (serialize) app info.
 */
kii_err_t kii_app_save(
        kii_app_t           *app,
        const kii_char_t    **out_data_ptr,
        int                 *out_data_len);

/**
 * Close an app.
 *
 * \param app Pointer to app.
 */
void kii_app_close(
        kii_app_t *app);

/***************************************************************************
 * Device
 */

kii_err_t kii_device_register(
        kii_app_t           *app,
        const kii_char_t    *dev_id,
        kii_json_t          *user_data);

/***************************************************************************
 * Objects
 */

/**
 * Add or replace an object.
 */
kii_err_t kii_object_put(
        kii_app_t           *app,
        kii_scope_t         scope,
        const kii_char_t    *bucket_name,
        const kii_char_t    *opt_object_id,
        kii_json_t          *object_data);

/**
 * Get an object from KiiCloud.
 */
kii_err_t kii_object_get(
        kii_app_t           *app,
        kii_scope_t         scope,
        const kii_char_t    *bucket_name,
        const kii_char_t    *object_id,
        kii_json_t          **out_object_data);

/**
 * Remove an object from KiiCloud.
 */
kii_err_t kii_object_delete(
        kii_app_t           *app,
        kii_scope_t         scope,
        const kii_char_t    *bucket_name,
        const kii_char_t    *object_id);

/***************************************************************************
 * Push notification (MQTT)
 */

/* TODO: TBD */

#ifdef __cplusplus
}
#endif

#endif /*KIICLOUD_INCLUDED*/
