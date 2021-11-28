#ifndef QRK_NET_TLS_H
#define QRK_NET_TLS_H

#include <quark/std/stream.h>
#include <quark/crypto/context.h>

#define QRK_SSL_IO_BUF_SIZE 1024
#define QRK_SSL_DEFAULT_CIPHER_SUITES "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256"
#define QRK_SSL_DEFAULT_CIPHER_LISTS "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA256:ECDHE-RSA-AES256-SHA384:DHE-RSA-AES256-SHA384:ECDHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA256:HIGH:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!SRP:!CAMELLIA"

typedef struct qrk_tls_s qrk_tls_t;

struct qrk_tls_s {
    QRK_MEMORY_CONTEXT_FIELDS
    QRK_ERR_FIELDS
    QRK_LOOP_HANDLE_FIELD
    QRK_STREAM_FIELDS
    qrk_stream_cb on_close;
    qrk_stream_t *stream;
    qrk_secure_ctx_t sec_ctx;
    SSL *ssl;
    BIO *rbio;
    BIO *wbio;
    qrk_str_t enc_buf;
};

void qrk_tls_read_underlying (qrk_stream_t *stream, qrk_rbuf_t *buf);
void qrk_tls_write (qrk_tls_t* tls, qrk_rbuf_t* buf);
int qrk_tls_init (qrk_tls_t* tls, qrk_stream_t* stream);
int qrk_tls_init2 (qrk_tls_t* tls, qrk_stream_t* stream, qrk_secure_ctx_t *sec_ctx);
int qrk_tls_connect (qrk_tls_t *tls);
int qrk_tls_accept (qrk_tls_t *tls);
int qrk_tls_connect_with_sni (qrk_tls_t *tls, const char *host);
int qrk_tls_shutdown (qrk_tls_t *tls);
/**
 * Notice: This method doesn't free the secure context, as it is reusable,
 * If you're not going to use it further you MUST call `qrk_sec_ctx_free`
 * before or after.
 */
void qrk_tls_free (qrk_tls_t *tls);

#endif //QRK_NET_TLS_H
