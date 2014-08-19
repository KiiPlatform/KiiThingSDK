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
#define KIICLOUD_MINOR_VERSION  0
#define KIICLOUD_MICRO_VERSION  0

#define KIICLOUD_VERSION    #KIICLOUD_MAJOR_VERSION "." #KIICLOUD_MICRO_VERSION
#define KIICLOUD_VERSION_HEX    ( \
        (KIICLOUD_MAJOR_VERSION << 16) | \
        (KIICLOUD_MINOR_VERSION <<  8) | \
        (KIICLOUD_MICRO_VERSION <<  0))

/***************************************************************************
 * Application
 */

typedef void* kii_app_t;

/**
 * Open an app.
 *
 * \param endpoint KiiCloud end point. ex. http://api.kii.com/api
 * \param app_id App ID.
 * \param app_key App key.
 */
kii_app_t* kii_app_open(char *endpoint, char *app_id, char *app_key);

/**
 * Close an app.
 *
 * \param app Pointer to app.
 */
void kii_app_close(kii_app_t *app);

/***************************************************************************
 * Device
 */

kii_device_register(kii_app_t *app, char* dev_id, json_t *user_data);

#ifdef __cplusplus
}
#endif

#endif /*KIICLOUD_INCLUDED*/
