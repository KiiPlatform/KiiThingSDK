#ifndef KiiThingSDK_kii_http_access_h
#define KiiThingSDK_kii_http_access_h

static kii_error_code_t prv_kii_http_delete(
        const kii_char_t* url,
        const kii_app_t* app,
        const kii_char_t* access_token,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error)
{
    // TODO: implement me.
    return KIIE_FAIL;
}

static kii_error_code_t prv_kii_http_get(
        const kii_char_t* url,
        const kii_app_t* app,
        const kii_char_t* access_token,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    // TODO: implement me.
    return KIIE_FAIL;
}

static kii_error_code_t prv_kii_http_head(
        const kii_char_t* url,
        const kii_app_t* app,
        const kii_char_t* access_token,
        long* response_status_code,
        kii_char_t** response_body,
        kii_error_t* error)
{
    // TODO: implement me.
    return KIIE_FAIL;
}

static kii_error_code_t prv_kii_http_patch(
        const kii_char_t* url,
        const kii_app_t* app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* opt_etag,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    // TODO: implement me.
    return KIIE_FAIL;
}

static kii_error_code_t prv_kii_http_post(
        const kii_char_t* url,
        const kii_app_t* app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    // TODO: implement me.
    return KIIE_FAIL;
}

static kii_error_code_t prv_kii_http_put(
        const kii_char_t* url,
        const kii_app_t* app,
        const kii_char_t* access_token,
        const kii_char_t* content_type,
        const kii_char_t* opt_etag,
        kii_bool_t if_none_match,
        const kii_char_t* request_body,
        long* response_status_code,
        json_t** response_headers,
        kii_char_t** response_body,
        kii_error_t* error)
{
    // TODO: implement me.
    return KIIE_FAIL;
}

#endif//KiiThingSDK_kii_http_access_h
