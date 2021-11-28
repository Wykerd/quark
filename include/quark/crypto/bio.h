#ifndef QRK_CRYPTO_BIO
#define QRK_CRYPTO_BIO

#ifndef qrk_CRYPTO_BIO_H
#define qrk_CRYPTO_BIO_H
#include <quark/std/alloc.h>
#include <openssl/err.h>
#include <openssl/bio.h>

typedef struct qrk_crypto_buffer_s {
    char* base;
    size_t len;
    size_t max;
    void* opaque;
} qrk_crypto_buffer_t;

typedef struct qrk_crypto_bio_s {
    void *data;
    qrk_malloc_ctx_t *m_ctx;
    qrk_crypto_buffer_t *buf;
    qrk_crypto_buffer_t *readp;
    int eof_return;
} qrk_crypto_bio_t;

const BIO_METHOD* qrk_crypto_bio_get_method ();
BIO* qrk_crypto_bio_new (qrk_malloc_ctx_t *ctx);
BIO* qrk_crypto_bio_new_from_buf (qrk_malloc_ctx_t *ctx, const char *base, size_t len);
BIO* qrk_crypto_bio_new_from_buf_fixed (qrk_malloc_ctx_t *ctx, const char *base, size_t len);

#endif

#endif //QRK_CRYPTO_BIO
