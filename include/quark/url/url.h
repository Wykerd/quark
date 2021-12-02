#ifndef QRK_URL_H
#define QRK_URL_H

#include <quark/std/buf.h>

void qrk_url_percent_decode (qrk_str_t *dest, qrk_rbuf_t *src);
void qrk_url_percent_encode (qrk_str_t *dest, qrk_rbuf_t *src, const uint8_t *set);
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
    QRK_URL_HOST_FAILED,
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

int qrk_url_host_parse (qrk_url_host_t *dest, qrk_rbuf_t *src, int is_not_special, int *validation_error);

#endif //QRK_URL_H
