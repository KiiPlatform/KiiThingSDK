#include <stdio.h>
#include <stdlib.h>

#include <kiicloud.h>

#define MY_APP_ID       "{your application ID}"
#define MY_APP_KEY      "{your application Key}"
#define MY_DEVICE_ID    "{your device ID}"
#define MY_APP_FILE     "/path/to/app/state_file"

kii_app_t *
first_setup() {
    kii_app_t   *app;
    kii_err_t   err;
    kii_char_t  *data_ptr = NULL;
    kii_int_t   data_len;
    FILE        *fp = NULL;
    size_t      written;

    /* Open an app struture (allocate memory). */
    app = kii_app_open();
    if (app) {
        fprintf(stderr, "something wrong at open\n");
        return NULL;
    }

    /* Setup ID and Key. */
    err = kii_app_init(app, "https://api.kii.com/api",
            MY_APP_ID, MY_APP_KEY);
    if (err != 0) {
        fprintf(stderr, "failed to init: %d\n", err);
        kii_app_close(app);
        return NULL;
    }

    /* Register a device without user data. */
    err = kii_device_register(app, MY_DEVICE_ID, NULL);
    if (err != 0) {
        fprintf(stderr, "failed to register a device: %d\n", err);
        kii_app_close(app);
        return NULL;
    }

    /* Persist application status. */

    /* Get serialized data. */
    err = kii_app_save(app, &data_ptr, &data_len);
    if (err != 0) {
        fprintf(stderr, "failed to save (serialize): %d\n", err);
        kii_app_close(app);
        return NULL;
    }

    /* Open a file to write. */
    fp = fopen(MY_APP_FILE, "wb");
    if (fp == NULL) {
        fprintf(stderr, "failed to open file: %s\n", MY_APP_FILE);
        free(data_ptr);
        kii_app_close(app);
        return NULL;
    }

    /* Write data to the file. */
    written = fwrite(data_ptr, 1, data_len, fp);
    if (written != data_len) {
        fprintf(stderr, "failed to write: %d\n", ferror(fp));
        fclose(fp);
        free(data_ptr);
        kii_app_close(app);
        return NULL;
    }

    /* dispose resources. */
    fclose(fp);
    free(data_ptr);

    return app;
}
