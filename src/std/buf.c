#include <quark/std/buf.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <quark/no_malloc.h>

void *qrk_str_malloc (qrk_str_t *buf, qrk_malloc_ctx_t *ctx, size_t initial_size)
{
    buf->m_ctx = ctx;
    buf->base = qrk_malloc(ctx, initial_size);
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
    qrk_free(buf->m_ctx, buf->base);
    buf->base = NULL;
    buf->size = 0;
    buf->len = 0;
}
