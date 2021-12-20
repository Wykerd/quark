#ifndef QRK_URL_H
#define QRK_URL_H

#include <quark/std/buf.h>

int qrk_url_percent_decode (qrk_str_t *dest, qrk_rbuf_t *src);
int qrk_url_percent_encode2 (qrk_str_t *dest, const char *src, size_t len, const uint8_t *set);
int qrk_url_percent_encode (qrk_str_t *dest, qrk_rbuf_t *src, const uint8_t *set);
/**
 *
 * @param dest
 * @param src
 * @param be_strict should default to false
 * @return non zero on failure
 */
int qrk_url_domain_to_ascii (qrk_str_t *dest, qrk_rbuf_t *src, int be_strict);

/**
 *
 * @param dest
 * @param src
 * @return non zero on failure
 */
int qrk_url_domain_to_unicode (qrk_str_t *dest, qrk_rbuf_t *src);

// TODO: int qrk_url_domain_validate (qrk_rbuf_t *domain);

typedef enum qrk_url_host_type {
    QRK_URL_HOST_NULL = 0,
    QRK_URL_HOST_EMPTY,
    QRK_URL_HOST_DOMAIN,
    QRK_URL_HOST_OPAQUE,
    QRK_URL_HOST_IPV4,
    QRK_URL_HOST_IPV6
} qrk_url_host_type_t;

typedef struct qrk_url_host_s {
    qrk_url_host_type_t type;
    qrk_str_t str;
    uint32_t ipv4;
    uint16_t ipv6[8];
} qrk_url_host_t;

int qrk_url_host_ipv4_number (uint32_t *dest, const char *src, size_t len, int *validation_error);
int qrk_url_host_ipv4_parse (uint32_t *dest, qrk_rbuf_t *src, int *validation_error);
int qrk_url_host_ipv6_parse (uint16_t *dest, qrk_rbuf_t *src, int *validation_error);
int qrk_url_host_opaque_parse (qrk_str_t *dest, qrk_rbuf_t *src, int *validation_error);
int qrk_url_host_parse (qrk_url_host_t *dest, qrk_rbuf_t *src, int is_not_special, int *validation_error);
int qrk_url_host_serialize (qrk_str_t *dest, qrk_url_host_t *src);
int qrk_url_host_init (qrk_url_host_t *host, qrk_malloc_ctx_t *ctx);
int qrk_url_host_clone (qrk_url_host_t *dest, qrk_url_host_t *src);
void qrk_url_host_free (qrk_url_host_t *host);

#define QRK_URL_HOST_IS_IPV4(host) ((host)->type == QRK_URL_HOST_IPV4)
#define QRK_URL_HOST_IS_IPV6(host) ((host)->type == QRK_URL_HOST_IPV6)
#define QRK_URL_HOST_IS_DOMAIN_OR_OPAQUE(host) ((host)->type == QRK_URL_HOST_DOMAIN || (host)->type == QRK_URL_HOST_OPAQUE)
#define QRK_URL_HOST_IS_DOMAIN(host) ((host)->type == QRK_URL_HOST_DOMAIN)
#define QRK_URL_HOST_IS_OPAQUE(host) ((host)->type == QRK_URL_HOST_OPAQUE)

// TODO: add oom error flag
typedef enum qrk_url_flags {
    QRK_URL_FLAG_NONE               = 0,
    QRK_URL_FLAG_INVALID            = 1 << 0,
    QRK_URL_FLAG_SPECIAL            = 1 << 1,
    QRK_URL_FLAG_HAS_USERNAME       = 1 << 2,
    QRK_URL_FLAG_HAS_PASSWORD       = 1 << 3,
    QRK_URL_FLAG_PATH_IS_OPAQUE     = 1 << 4,
    QRK_URL_FLAG_IS_FILE            = 1 << 5,
    QRK_URL_FLAG_VALIDATION_ERROR   = 1 << 6
} qrk_url_flags_t;

typedef struct qrk_url_s {
    qrk_url_flags_t flags;
    qrk_str_t scheme;
    qrk_str_t username;
    qrk_str_t password;
    qrk_url_host_t host;
    int32_t port; // port is negative when null
    qrk_buf_t path;
    qrk_str_t query;
    qrk_str_t fragment;
    // TODO: blob URL entry
} qrk_url_t;

typedef enum qrk_url_parser_state {
    QRK_URL_PARSER_STATE_NO_OVERRIDE = 0,
    QRK_URL_PARSER_STATE_SCHEME_START,
    QRK_URL_PARSER_STATE_SCHEME,
    QRK_URL_PARSER_STATE_NO_SCHEME,
    QRK_URL_PARSER_STATE_SPECIAL_RELATIVE_OR_AUTHORITY,
    QRK_URL_PARSER_STATE_PATH_OR_AUTHORITY,
    QRK_URL_PARSER_STATE_RELATIVE,
    QRK_URL_PARSER_STATE_RELATIVE_SLASH,
    QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_SLASHES,
    QRK_URL_PARSER_STATE_SPECIAL_AUTHORITY_IGNORE_SLASHES,
    QRK_URL_PARSER_STATE_AUTHORITY,
    QRK_URL_PARSER_STATE_HOST,
    QRK_URL_PARSER_STATE_HOSTNAME,
    QRK_URL_PARSER_STATE_PORT,
    QRK_URL_PARSER_STATE_FILE,
    QRK_URL_PARSER_STATE_FILE_SLASH,
    QRK_URL_PARSER_STATE_FILE_HOST,
    QRK_URL_PARSER_STATE_PATH_START,
    QRK_URL_PARSER_STATE_PATH,
    QRK_URL_PARSER_STATE_OPAQUE_PATH,
    QRK_URL_PARSER_STATE_QUERY,
    QRK_URL_PARSER_STATE_FRAGMENT
} qrk_url_parser_state_t;

typedef struct qrk_url_parser_s {
    QRK_MEMORY_CONTEXT_FIELDS
    qrk_url_parser_state_t state;
} qrk_url_parser_t;

// TODO: this is part of the HTML spec and should be moved
typedef struct qrk_html_origin_tuple_s {
    qrk_str_t scheme;
    qrk_url_host_t host;
    int32_t port;
    qrk_str_t domain;
} qrk_html_origin_tuple_t;

int qrk_html_origin_tuple_init (qrk_html_origin_tuple_t *tuple, qrk_malloc_ctx_t *mctx);
void qrk_html_origin_tuple_free(qrk_html_origin_tuple_t *tuple);
int qrk_html_origin_serialize (qrk_str_t *str, qrk_html_origin_tuple_t *tuple);

// TODO: we still need to add blob support
// TODO: int qrk_url_parse (qrk_url_parser_t *parser, qrk_rbuf_t *url, qrk_url_t *base);
int qrk_url_parse_basic (qrk_url_parser_t *parser, qrk_rbuf_t *input, qrk_url_t *base,
                         qrk_url_t **url, qrk_url_parser_state_t state_override);
int qrk_url_serialize (qrk_url_t *url, qrk_str_t *str, int exclude_fragment);
int qrk_url_origin (qrk_url_t *url, qrk_html_origin_tuple_t **origin, qrk_malloc_ctx_t *mctx);
int qrk_url_parser_init (qrk_url_parser_t *parser, qrk_malloc_ctx_t *ctx);
void qrk_url_free (qrk_url_t *url);

int qrk_url_form_urlencoded_parse (qrk_rbuf_t *src, qrk_buf_t *list, qrk_malloc_ctx_t *mctx);

#endif //QRK_URL_H
