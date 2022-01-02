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

/// Object buffer
/**
 * Allocate a new buffer with the given properties.
 * @param buf The buffer to initialize.
 * @param ctx Memory context to use for allocating the buffer.
 * @param memb_size Size of each element in the buffer.
 * @param nmemb Initial number of elements to allocate.
 * @return Pointer to first element in the buffer.
 */
void *qrk_buf_malloc (qrk_buf_t *buf, qrk_malloc_ctx_t *ctx, size_t memb_size, size_t nmemb);
/**
 * Resize a buffer.
 * @param buf The buffer to resize.
 * @param nmemb New number of elements to allocate.
 * @return Pointer to first element in the buffer.
 */
void *qrk_buf_resize (qrk_buf_t *buf, size_t nmemb);
/**
 * Add elements to back of buffer.
 * @param buf The target buffer.
 * @param src Array of elements to add.
 * @param nmemb Number of elements to add.
 * @return Pointer to first element added.
 */
void *qrk_buf_push_back (qrk_buf_t *buf, const void **src, size_t nmemb);
/**
 * Add single element to back of buffer.
 * @param buf The target buffer.
 * @param src Element to add.
 * @returns Pointer to element added.
 */
void *qrk_buf_push_back1 (qrk_buf_t *buf, const void *src);
/**
 * Add elements to front of buffer.
 * @param buf The target buffer.
 * @param src Array of elements to add.
 * @param nmemb Number of elements to add.
 * @return Pointer to first element added.
 */
void *qrk_buf_push_front (qrk_buf_t *buf, const void **src, size_t nmemb);
/**
 * Remove elements from front of buffer.
 * @param buf The target buffer.
 * @param nmemb Number of elements to remove.
 * @return Pointer to first element removed.
 */
void qrk_buf_shift (qrk_buf_t *buf, size_t nmemb);
/**
 * Free a buffer.
 * @param buf The buffer to free.
 */
void qrk_buf_free (qrk_buf_t *buf);
/**
 * Get element at given index.
 * @param buf Target buffer.
 * @param i Index
 * @return Pointer to element at given index.
 */
void *qrk_buf_get (qrk_buf_t *buf, size_t i);
/**
 * Remove element at given index.
 * @param buf Target buffer.
 * @param i Index
 */
void qrk_buf_remove (qrk_buf_t *buf, size_t i);

#define QRK_STR_PUTC_GROWTH_FACTOR 2

/// String buffer
void *qrk_str_malloc (qrk_str_t *buf, qrk_malloc_ctx_t *ctx, size_t initial_size);
void *qrk_str_resize (qrk_str_t *buf, size_t size);
void *qrk_str_push_back (qrk_str_t *buf, const char *src, size_t len);
void *qrk_str_putc (qrk_str_t *buf, char c);
void *qrk_str_push_front (qrk_str_t *buf, const char *src, size_t len);
void qrk_str_shift (qrk_str_t *buf, size_t len);
void *__attribute__((format(printf, 2, 3))) qrk_str_printf (qrk_str_t *buf, const char *fmt, ...);
void *qrk_str_puts (qrk_str_t *buf, const char *s);
void qrk_str_free (qrk_str_t *buf);

#endif
