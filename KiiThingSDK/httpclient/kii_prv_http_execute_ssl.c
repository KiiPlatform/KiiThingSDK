#include "../kii_http_adapter.h"
#include "../kii_custom.h"
#include "../kii_prv_utils.h"
#include "../kii_prv_types.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* for UNIX like systems */
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* Use OpenSSL */
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define SOCKET_CLOSE(s) close(s)
#define HTTP_CONFIG_DEFAULTPORT 80
#define HTTP_CONFIG_URLMAXPATH 256
#define HTTP_EXCONFIG_PRINTBUFFER 1024 /* affect to stack size */
#define HTTP_EXCONFIG_HEADERMAXCOUNT 1024
#define HTTP_EXCONFIG_HEADERLINEBLOCKSIZE 256

typedef enum {
    HTTP_RESULT_OK = 0,
    HTTP_RESULT_ERROR_RESPONSEHEADER,
    HTTP_RESULT_ERROR_RECEIVING,
    HTTP_RESULT_ERROR_CONNECTSERVER,
    HTTP_RESULT_ERROR_URLSYNTAX,
    HTTP_RESULT_ERROR_INTERNAL,
    HTTP_RESULT_ERROR_CONNECTSSLSERVER,
    HTTP_RESULT_ERROR_UNKNOWN
} http_result_t;

typedef struct {
    kii_char_t host[256];
    kii_int_t port;
    kii_char_t path[HTTP_CONFIG_URLMAXPATH];
} http_url_t;

static http_result_t http_error_code;

static kii_int_t
parse_url(
        http_url_t* url,
        const kii_char_t* str)
{
    const kii_char_t* end;
    const kii_char_t* top1 = NULL;
    const kii_char_t* end1 = NULL;
    const kii_char_t* top2 = NULL;
    const kii_char_t* end2 = NULL;
    const kii_char_t* top3 = NULL;
    const kii_char_t* end3 = NULL;
    if (str == NULL)
        return 0; /* invalid URL string */
    end = str + strlen(str);
    memset(url, 0, sizeof(*url));
    /* check schema */
    if (strncmp(str, "http://", 7) == 0)
        top1 = &str[7];
    else if (strncmp(str, "https://", 8) == 0)
        top1 = &str[8];
    if (top1 == NULL)
        return 0; /* unsupported schema */
    /* search end of HOST and PORT (end2) */
    end2 = strchr(top1, '/');
    if (end2 == top1)
        return 0; /* without HOST or PORT */
    else if (end2 == NULL)
    {
        end2 = end;
        top3 = NULL;
    }
    else
        top3 = end2;
    /* split PORT from HOST and PORT */
    end1 = strchr(top1, ':');
    if (end1 == NULL || end1 > end2)
        end1 = end2;
    else
        top2 = end1 + 1;
    /* FIXME: split PATH from the remain (QUERY_STRING) */
    end3 = end;
    /* check length */
    if (end1 - top1 >= sizeof(url->host))
        return 0; /* too long HOST */
    if (top3 != NULL && end3 != NULL && end3 - top3 >= sizeof(url->path))
        return 0; /* too long PATH */
    /* copy to output */
    strncpy(url->host, top1, end1 - top1);
    url->port = top2 ? atoi(top2) : HTTP_CONFIG_DEFAULTPORT;
    if (top3)
        strncpy(url->path, top3, end3 - top3);
    else
        url->path[0] = '/';
    return 1;
}

static void
socket_close(
        kii_int_t sock)
{
    SOCKET_CLOSE(sock);
}

static kii_int_t
socket_connect(
        const http_url_t* url)
{
    kii_int_t sock = -1;
    struct hostent* entry;
    in_addr_t address;
    struct in_addr** p_addr;
    struct sockaddr_in addr;
    struct servent* service;
    /* convert hostname to IP address */
    if ((entry = gethostbyname(url->host)) != NULL)
        address = (in_addr_t)entry->h_addr;
    else if ((address = inet_addr(url->host)) == INADDR_NONE)
    {
        sock = -2;
        goto END_FUNC;
    }
    service = getservbyname("https", "tcp");
    /* create and connect socket to host */
    for (p_addr = (struct in_addr**)entry->h_addr_list;
            *p_addr != NULL; ++p_addr)
    {
        /* create socket */
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1)
            goto END_FUNC;
        /* connect socket to host */
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        if (service != NULL)
        {
            addr.sin_port = service->s_port;
            //addr.sin_port = htons(443);
        }
        else
        {
            addr.sin_port = htons((uint16_t)url->port);
        }
        memcpy(&addr.sin_addr, *p_addr, sizeof(struct in_addr));
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != -1)
            goto END_FUNC; /* success */
        socket_close(sock);
        sock = -1;
    }
    sock = -3; /* error becase of host down */
END_FUNC:
    return sock;
}

static void
ssl_write(
        SSL* ssl,
        const kii_char_t* ptr,
        kii_int_t len,
        kii_int_t binary)
{
    SSL_write(ssl, ptr, len);
}

static kii_int_t
ssl_reqhdr_printf(
        SSL* ssl,
        const kii_char_t* fmt,
        ...)
{
    kii_int_t retval;
    va_list list;
    kii_char_t buf[HTTP_EXCONFIG_PRINTBUFFER];
    /* format message */
    va_start(list, fmt);
    retval = vsnprintf(buf, sizeof(buf), fmt, list);
    va_end(list);
    /* FIXME: compare retval with sizeof(buf) */
    ssl_write(ssl, (kii_char_t*)buf, retval, 0);
    return retval;
}

static kii_int_t
ssl_readbyte(SSL* ssl)
{
    /* detect read timeout */
    {
        fd_set fds;
        kii_int_t sock;
        struct timeval timeout;
        kii_int_t nfds;
        FD_ZERO(&fds);
        sock = SSL_get_fd(ssl);
        FD_SET(sock, &fds);
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;
        nfds = select(sock + 1, &fds, NULL, NULL, &timeout);
        if (nfds == 0)
        {
            return -1; /* timeout error */
        }
        else if (nfds != 1)
            return -2; /* system problem, must not reached */
    }
    /* read a byte from socket */
    {
        kii_char_t buf[1];
        kii_int_t len;
        len = SSL_read(ssl, buf, 1);
        if (len != 1)
        {
            return -3; /* system problem, must not reached */
        }
        return buf[0];
    }
}

static kii_int_t
ssl_resphdr_readline(
        SSL* ssl,
        kii_char_t** bufptr)
{
    kii_int_t retval = -3; /* means too small buffer */
    kii_int_t index = 0;
    kii_int_t buflen = HTTP_EXCONFIG_HEADERLINEBLOCKSIZE;
    *bufptr = kii_malloc(sizeof(kii_char_t) * buflen);
    memset(*bufptr, 0, sizeof(kii_char_t) * buflen);
    while (1)
    {
        kii_int_t d1;
        kii_char_t d;
        d1 = ssl_readbyte(ssl);
        if (d1 < 0)
        {
            retval = d1;
            break;
        }

        d = d1;

        if (d1 == '\n')
        {
            /* NOTE: this is invaild sequence as a line */
            retval = index;
            break;
        }
        else if (d1 == '\r')
        {
            kii_int_t d2;
            d2 = ssl_readbyte(ssl);
            if (d2 < 0)
            {
                retval = d2;
                break;
            }
            d = d2;
            if (d2 == '\n')
            {
                retval = index;
                break;
            }
            else if (index + 1 < buflen)
            {
                (*bufptr)[index++] = (kii_char_t)d1;
                (*bufptr)[index++] = (kii_char_t)d2;
            }
            else
            {
                /* too small buffer */
                buflen += HTTP_EXCONFIG_HEADERLINEBLOCKSIZE;
                *bufptr = kii_realloc(*bufptr, sizeof(kii_char_t) * buflen);
                (*bufptr)[index++] = (kii_char_t)d1;
                (*bufptr)[index++] = (kii_char_t)d2;
            }
        }
        else if (index < buflen)
            (*bufptr)[index++] = (kii_char_t)d1;
        else
        {
            /* too small buffer */
            buflen += HTTP_EXCONFIG_HEADERLINEBLOCKSIZE;
            *bufptr = kii_realloc(*bufptr, sizeof(kii_char_t) * buflen);
            (*bufptr)[index++] = (kii_char_t)d1;
        }
    }
    if (retval <= 0)
    {
        M_KII_FREE_NULLIFY(*bufptr);
        *bufptr = NULL;
    }
    return retval;
}

static http_result_t
ssl_send_request(
        SSL* ssl,
        const kii_char_t* method,
        const http_url_t* url,
        json_t* request_headers,
        const kii_char_t* req_bufptr)
{
    /* output HTTP request header to ssl socket */
    kii_int_t with_data = 0;
    const kii_char_t* str_target = NULL;
    const kii_char_t* str_host = NULL;
    const char* header_key = NULL;
    json_t* header_value = NULL;
    kii_int_t req_buflen = 0;

    if (req_bufptr != NULL)
    {
        with_data = 1;
        req_buflen = (kii_int_t)kii_strlen(req_bufptr);
    }
    /* FIXME: consider HTTP PROXY */
    str_target = url->path;
    str_host = url->host;
    /* FIXME: consider error in send */
    ssl_reqhdr_printf(ssl, "%s %s HTTP/1.1\r\n", method, str_target);
    ssl_reqhdr_printf(ssl, "Host:%s\r\n", str_host);
    json_object_foreach(request_headers, header_key, header_value)
    {
        ssl_reqhdr_printf(ssl, "%s:%s\r\n", header_key,
                json_string_value(header_value));
        M_KII_DEBUG(prv_log("req header: %s:%s", header_key,
                    json_string_value(header_value)));
    }
    /* FIXME: implement send proxy authorization information */
    ssl_reqhdr_printf(ssl, "Connection:close\r\n");
    if (with_data)
    {
        ssl_reqhdr_printf(ssl, "Content-Length:%d\r\n", req_buflen);
    }
    /* send request terminator */
    ssl_reqhdr_printf(ssl, "\r\n");
    /* send payload (ex. POST method) */
    if (with_data)
        ssl_write(ssl, req_bufptr, req_buflen, 1);
    return HTTP_RESULT_OK;
}

static http_result_t
ssl_recv_response(
        SSL* ssl,
        kii_int_t* status,
        kii_char_t** response_body,
        json_t** response_headers)
{
    http_result_t retval = HTTP_RESULT_OK;
    kii_int_t bodylen = -1;

    /* receive and parse HTTP response header */
    {
        kii_int_t count;
        /* fetch HTTP status code */
        for (count = 0; count < HTTP_EXCONFIG_HEADERMAXCOUNT; ++count)
        {
            kii_char_t* line = NULL;
            kii_int_t len;
            len = ssl_resphdr_readline(ssl, &line);
            if (len < 0)
            {
                /* FIXME: convert to socket read error */
                retval = HTTP_RESULT_ERROR_RECEIVING;
                M_KII_FREE_NULLIFY(line);
                goto END_FUNC;
            }
            else if (len == 0)
            {
                M_KII_FREE_NULLIFY(line);
                continue;
            }
            if (strncmp((char*)line, "HTTP/1.1 ", 9) != 0)
            {
                retval = HTTP_RESULT_ERROR_RESPONSEHEADER;
                M_KII_FREE_NULLIFY(line);
                goto END_FUNC;
            }
            else if (!isdigit((int)line[9]))
            {
                retval = HTTP_RESULT_ERROR_RESPONSEHEADER;
                M_KII_FREE_NULLIFY(line);
                goto END_FUNC;
            }
            else
            {
                *status = atoi((char*)&line[9]);
                M_KII_FREE_NULLIFY(line);
                if (*status == 100)
                    continue;
                else
                    break;
            }
        }
        if (count >= HTTP_EXCONFIG_HEADERMAXCOUNT)
        {
            retval = HTTP_RESULT_ERROR_RESPONSEHEADER;
            goto END_FUNC;
        }
        /* fetch HTTP response header properties */
        for (count = 0; count < HTTP_EXCONFIG_HEADERMAXCOUNT; ++count)
        {
            int i;
            kii_char_t* line = NULL;
            kii_int_t len;
            len = ssl_resphdr_readline(ssl, &line);
            if (len < 0)
            {
                /* FIXME: convert to socket read error */
                retval = HTTP_RESULT_ERROR_RECEIVING;
                M_KII_FREE_NULLIFY(line);
                goto END_FUNC;
            }
            else if (len == 0)
            {
                M_KII_FREE_NULLIFY(line);
                break;
            }

            M_KII_DEBUG(prv_log("resp header: %s", line));
            for (i = 0; line[i] != '\0' && line[i] != ':'; ++i) {
                line[i] = (char)kii_tolower(line[i]);
            }
            if (strncmp((char*)line, "content-length:", 15) == 0)
            {
                /* FIXME: more strict check */
                bodylen = atoi((char*)&line[15]);
            }
            else if (response_headers != NULL &&
                    strncmp((char*)line, "etag:", 5) == 0)
            {
                if (*response_headers == NULL)
                    *response_headers = json_object();
                if (json_object_set_new(*response_headers, "etag",
                            json_string(&line[5])) != 0)
                {
                    retval = HTTP_RESULT_ERROR_RESPONSEHEADER;
                    M_KII_FREE_NULLIFY(line);
                    goto END_FUNC;
                }
            }
            /* FIXME: parse other properties */
            M_KII_FREE_NULLIFY(line);
        }
    }
    /* receive HTTP response body */
    if (response_body != NULL)
    {
        kii_char_t* wptr;
        kii_char_t* end;
        *response_body = kii_malloc(sizeof(kii_char_t) * (bodylen + 1));
        wptr = *response_body;
        end = wptr + bodylen;
        for (; wptr < end; ++wptr)
        {
            kii_int_t d;
            d = ssl_readbyte(ssl);
            if (d < 0)
            {
                /* FIXME: convert to socket read error */
                retval = HTTP_RESULT_ERROR_RECEIVING;
                goto END_FUNC;
            }
            *wptr = (kii_char_t)d;
        }
        (*response_body)[bodylen] = '\0';
        M_KII_DEBUG(prv_log("response: %s", *response_body));
    }
END_FUNC:
    return retval;
}

static int32_t
ssl_connect(kii_int_t socket, SSL_CTX** out_ctx, SSL** out_ssl)
{
    int32_t ret = 0;
    SSL_load_error_strings();
    SSL_library_init();

    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if ( ctx == NULL )
    {
        goto END_FUNC;
    }

    SSL *ssl = SSL_new(ctx);
    if ( ssl == NULL )
    {
        goto END_FUNC;
    }

    ret = SSL_set_fd(ssl, socket);
    if (ret == 0)
    {
        goto END_FUNC;
    }

    RAND_poll();
    while(RAND_status() == 0)
    {
        unsigned short rand_ret = rand() % 65536;
        RAND_seed(&rand_ret, sizeof(rand_ret));
    }

    ret = SSL_connect(ssl);

END_FUNC:
    if (ret != 0)
    {
        *out_ctx = ctx;
        *out_ssl = ssl;
    }
    else
    {
        SSL_free(ssl); 
        SSL_CTX_free(ctx);
    }
    return ret;
}

static http_result_t
request(const kii_char_t* method,
        const kii_char_t* urlstr,
        json_t* request_headers,
        const kii_char_t* request_body,
        kii_int_t* status_code,
        json_t** response_headers,
        kii_char_t** response_body)
{
    http_result_t retval = HTTP_RESULT_ERROR_INTERNAL;
    http_url_t url;
    kii_int_t sock = 0;
    SSL_CTX* ctx = NULL;
    SSL* ssl = NULL;

    M_KII_DEBUG(prv_log("request url: %s", urlstr));
    M_KII_DEBUG(prv_log("request method: %s", method));
    M_KII_DEBUG(prv_log("request body: %s", request_body));

    if (!parse_url(&url, urlstr))
    {
        retval = HTTP_RESULT_ERROR_URLSYNTAX;
        goto END_FUNC;
    }
    /* connect to HTTP server */
    sock = socket_connect(&url);
    if (sock < 0)
    {
        retval = HTTP_RESULT_ERROR_CONNECTSERVER;
        goto END_FUNC;
    }
    if (ssl_connect(sock, &ctx, &ssl) != 1)
    {
        retval = HTTP_RESULT_ERROR_CONNECTSSLSERVER;
        goto END_FUNC;
    }
    /* output HTTP request header to socket */
    retval = ssl_send_request(ssl, method, &url, request_headers, request_body);
    if (retval != HTTP_RESULT_OK)
    {
        goto END_FUNC;
    }
    /* receive HTTP response and parse it */
    retval = ssl_recv_response(ssl, status_code, response_body,
            response_headers);
END_FUNC:
    if (ssl != NULL)
        SSL_shutdown(ssl);
    if (sock >= 0)
        socket_close(sock);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    ERR_free_strings();
    return retval;
}

kii_bool_t kii_http_init(void)
{
    // Nothing to implement.
    return KII_TRUE;
}

void kii_http_cleanup(void)
{
    // Nothing to implement.
}

kii_bool_t kii_http_execute(
        const kii_char_t* http_method,
        const kii_char_t* url,
        json_t* request_headers,
        const kii_char_t* request_body,
        kii_int_t* status_code,
        json_t** response_headers,
        kii_char_t** response_body)
{
    http_error_code = request(http_method, url, request_headers, request_body,
            status_code, response_headers, response_body);
    return (http_error_code == HTTP_RESULT_OK) ? KII_TRUE : KII_FALSE;
}
 
