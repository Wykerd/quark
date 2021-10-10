#ifndef QRK_STD_BUF_H
#define QRK_STD_BUF_H

#include <quark/std/alloc.h>

// Compatible with uv_but_t
typedef struct qrk_rbuf_s {
	char *base;
	size_t len;
} qrk_rbuf_t;

// Compatible with uv_buf_t
typedef struct qrk_str_s {
    char *base;
    size_t len;
    size_t size;
    qrk_malloc_ctx_t *m_ctx;
} qrk_str_t;

typedef struct qrk_buf_s {
    void *base;
    size_t nmemb;
    size_t size;
    qrk_malloc_ctx_t *m_ctx;
    size_t memb_size;
} qrk_buf_t;

void *qrk_buf_malloc (qrk_buf_t *buf, qrk_malloc_ctx_t *ctx, size_t memb_size, size_t nmemb);
void *qrk_buf_resize (qrk_buf_t *buf, size_t nmemb);
void *qrk_buf_push_back (qrk_buf_t *buf, const void **src, size_t nmemb);
void *qrk_buf_push_front (qrk_buf_t *buf, const void **src, size_t nmemb);
void qrk_buf_shift (qrk_buf_t *buf, size_t nmemb);
void qrk_buf_free (qrk_buf_t *buf);
void *qrk_buf_get (qrk_buf_t *buf, size_t i);

void *qrk_str_malloc (qrk_str_t *buf, qrk_malloc_ctx_t *ctx, size_t initial_size);
void *qrk_str_resize (qrk_str_t *buf, size_t size);
void *qrk_str_push_back (qrk_str_t *buf, const char *src, size_t len);
void *qrk_str_push_front (qrk_str_t *buf, const char *src, size_t len);
void qrk_str_shift (qrk_str_t *buf, size_t len);
void *__attribute__((format(printf, 2, 3))) qrk_str_printf (qrk_str_t *buf, const char *fmt, ...);
void *qrk_str_puts (qrk_str_t *buf, const char *s);
void qrk_str_free (qrk_str_t *buf);

#endif
