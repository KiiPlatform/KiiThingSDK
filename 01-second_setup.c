#include <stdio.h>
#include <stdlib.h>

#include <kiicloud.h>

#define MY_APP_FILE     "/path/to/app/state_file"

kii_app_t *
second_setup() {
    kii_app_t   *app;
    FILE        *fp = NULL;
    fpos_t      fsize;
    kii_char_t  *data_ptr = NULL;
    size_t      read;
    kii_err_t   err;

    /* Open an app struture (allocate memory). */
    app = kii_app_open();
    if (app) {
        fprintf(stderr, "something wrong at open\n");
        return NULL;
    }

    /* Open a file which have app state. */
    fp = fopen(MY_APP_FILE, "rb");
    if (fp == NULL) {
        fprintf(stderr, "failed to open file: %s\n", MY_APP_FILE);
        kii_app_close(app);
        return NULL;
    }

    /* Get file size. */
    fseek(fp, 0, SEEK_END);
    fgetpos(fp, &fsize);
    fseek(fp, 0, SEEK_SET);

    /* Allocate memory to read the file. */
    data_ptr = (kii_char_t*)malloc((size_t)fsize);
    if (data_ptr == NULL) {
        fprintf(stderr, "failed to allocate memory\n");
        fclose(fp);
        kii_app_close(app);
        return NULL;
    }

    /* Read file. */
    read = fread(data_ptr, 1, (size_t)fsize, fp);
    if (read != fsize) {
        fprintf(stderr, "failed to read a file: %d\n", ferror(fp));
        free(data_ptr);
        fclose(fp);
        kii_app_close(app);
        return NULL;
    }
    fclose(fp);

    err = kii_app_load(app, data_ptr, (kii_int_t)fsize);
    if (err != 0) {
        fprintf(stderr, "can't load as app: %d\n", err);
        free(data_ptr);
        kii_app_close(app);
        return NULL;
    }

    free(data_ptr);

    return app;
}
