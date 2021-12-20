#include <quark/std/buf.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <quark/no_malloc.h>

void *qrk_str_malloc (qrk_str_t *buf, qrk_malloc_ctx_t *ctx, size_t initial_size)
{
    buf->m_ctx = ctx;
    buf->base = qrk_malloc(ctx, initial_size ? initial_size : 1);
    buf->len = 0;
    buf->size = initial_size;
    return buf->base;
}

void *qrk_str_resize (qrk_str_t *buf, size_t size)
{
    void *new_ptr = qrk_realloc(buf->m_ctx, buf->base, size);
    if (!new_ptr)
        return NULL;
    buf->base = new_ptr;
    buf->size = size;
    if (buf->len > buf->size)
        buf->len = buf->size;
    return buf->base;
}

void *qrk_str_push_back (qrk_str_t *buf, const char *src, size_t len)
{
    if (buf->size < (buf->len + len))
        if (!qrk_str_resize(buf, buf->len + len))
            return NULL;
        
    char *sp = buf->base + buf->len;

    memcpy(sp, src, len);
    buf->len += len;

    return sp; 
}

void *qrk_str_putc (qrk_str_t *buf, char c)
{
    if (buf->size < (buf->len + 1))
        if (!qrk_str_resize(buf, buf->len * QRK_STR_PUTC_GROWTH_FACTOR))
            return NULL;

    char *sp = buf->base + buf->len;
    *sp = c;
    buf->len++;

    return sp;
}

void *__attribute__((format(printf, 2, 3))) qrk_str_printf (qrk_str_t *buf, const char *fmt, ...)
{
    va_list ap;
    int len;
    char buff[256];

    va_start(ap, fmt);
    len = vsnprintf(buff, sizeof(buff), fmt, ap);
    va_end(ap);
    if (len < sizeof(buff))
        return qrk_str_push_back(buf, buff, len);

    if (buf->size < (buf->len + len + 1))
        if (!qrk_str_resize(buf, buf->len + len + 1))
            return NULL;
    
    char *sp = buf->base + buf->len;

    va_start(ap, fmt);
    vsnprintf(sp, buf->size - buf->len, fmt, ap);
    va_end(ap);

    buf->len += len;

    return sp; 
}

void *qrk_str_puts (qrk_str_t *buf, const char *s)
{
    return qrk_str_push_back(buf, s, strlen(s));
}

void *qrk_str_push_front (qrk_str_t *buf, const char *src, size_t len)
{
    if (buf->size < (buf->len + len))
        if (!qrk_str_resize(buf, buf->len + len))
            return NULL;
    for (size_t i = 0; i < buf->len; i++)
        buf->base[buf->len - 1 + len - i] = buf->base[buf->len - 1 - i];
    
    memcpy(buf->base, src, len);
    buf->len += len;

    return buf->base;
}

void qrk_str_shift (qrk_str_t *buf, size_t len)
{
    memmove(buf->base, buf->base+len, buf->len-len);
    buf->len -= len;
}

void qrk_str_free (qrk_str_t *buf)
{
    if (buf->base)
        qrk_free(buf->m_ctx, buf->base);

    buf->base = NULL;
    buf->size = 0;
    buf->len = 0;
}

void *qrk_buf_malloc (qrk_buf_t *buf, qrk_malloc_ctx_t *ctx,
					  size_t memb_size, size_t nmemb) {
	buf->m_ctx = ctx;
	buf->memb_size = memb_size;
	buf->base = qrk_malloc(ctx, buf->memb_size * nmemb);
	buf->nmemb = 0;
	buf->size = nmemb;
	return buf->base;
}

void *qrk_buf_resize (qrk_buf_t *buf, size_t nmemb) {
	void *new_ptr = qrk_realloc(buf->m_ctx, buf->base, buf->memb_size * nmemb);
	if (!new_ptr)
		return NULL;
	buf->base = new_ptr;
	buf->size = nmemb;
	if (buf->nmemb > buf->size)
		buf->nmemb = buf->size;
	return buf->base;
}

void *qrk_buf_push_back (qrk_buf_t *buf, const void **src, size_t nmemb) {
	if (buf->size < (buf->nmemb + nmemb))
		if (!qrk_buf_resize(buf, buf->nmemb + nmemb))
			return NULL;

	char *sp = buf->base + (buf->nmemb * buf->memb_size);

	for (size_t i = 0; i < nmemb; ++i)
		memcpy(sp + (i * buf->memb_size), src[i], buf->memb_size);

	buf->nmemb += nmemb;

	return sp;
}

void *qrk_buf_push_back1 (qrk_buf_t *buf, const void *src)
{
    if (buf->size < (buf->nmemb + 1))
        if (!qrk_buf_resize(buf, buf->nmemb + 1))
            return NULL;

    char *sp = buf->base + (buf->nmemb * buf->memb_size);

    memcpy(sp, src, buf->memb_size);

    buf->nmemb++;

    return sp;
}

void *qrk_buf_push_front (qrk_buf_t *buf, const void **src, size_t nmemb) {
	if (buf->size < (buf->nmemb + nmemb))
		if (!qrk_buf_resize(buf, buf->nmemb + nmemb))
			return NULL;
	for (size_t i = 0; i < buf->nmemb; ++i)
		memcpy(buf->base + (((buf->nmemb - 1 - i) + nmemb) * buf->memb_size), buf->base + ((buf->nmemb - 1 - i) * buf->memb_size), buf->memb_size);

	for (size_t i = 0; i < nmemb; ++i)
		memcpy(buf->base + (i * buf->memb_size), src[i], buf->memb_size);

	buf->nmemb += nmemb;

	return buf->base;
}

void qrk_buf_shift (qrk_buf_t *buf, size_t nmemb)
{
	memmove(buf->base, buf->base + (nmemb * buf->memb_size), (buf->nmemb - nmemb) * buf->memb_size);
	buf->nmemb -= nmemb;
}

void qrk_buf_free (qrk_buf_t *buf)
{
    if (buf->base)
	    qrk_free(buf->m_ctx, buf->base);
	buf->base = NULL;
	buf->size = 0;
	buf->nmemb = 0;
}

void *qrk_buf_get (qrk_buf_t *buf, size_t i) {
	if (i > buf->nmemb - 1)
		return NULL;
	return buf->base + (i * buf->memb_size);
}
