#include <quark/std/alloc.h>
#include <quark/std/utils.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Memory allocation implementation is from QuickJS, 
 * which is licenced under the MIT license. It is available here:
 * https://bellard.org/quickjs/
 */
#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#endif
#if defined(__APPLE__)
#define MALLOC_OVERHEAD  0
#else
#define MALLOC_OVERHEAD  8
#endif
#include <assert.h>

static inline size_t qrk__def_malloc_usable_size(void *ptr)
{
#if defined(__APPLE__)
    return malloc_size(ptr);
#elif defined(_WIN32)
    return _msize(ptr);
#elif defined(EMSCRIPTEN)
    return 0;
#elif defined(__linux__)
    return malloc_usable_size(ptr);
#else
    /* change this to `return 0;` if compilation fails */
    return malloc_usable_size(ptr);
#endif
}

#if defined(QRK_MALLOC_DEBUG) && defined(__linux__)
#include <execinfo.h>
static void qrk__generate_trace (qrk_malloc_state_t *s, void *ptr) 
{
    char **strings;
    size_t i, size;
    void *array[1024];
    size = backtrace(array, 1024);
    strings = backtrace_symbols(array, size);

    size_t trace_len = 0;

    for (i = 1; i < size; i++)
        trace_len += sizeof("   at ") + strlen(strings[i]);

    char *trace = calloc(trace_len + 1, 1);
    size_t cursor = 0;

    for (i = 2; i < size; i++)
    {
        sprintf(trace + cursor, "   at %s\n", strings[i]);
        cursor += sizeof("   at ") + strlen(strings[i]);
    }

    qrk_hashmap_set(&s->pointers, ptr, trace);

    free(strings);
}
static void qrk__remove_trace (qrk_malloc_state_t *s, void *ptr) 
{
    void *trace = qrk_hashmap_delete(&s->pointers, ptr);
    if (trace)
        free(trace);
}
#else
#define qrk__generate_trace(s, ptr)
#define qrk__remove_trace(s, ptr)
#endif

static void *qrk__def_malloc(qrk_malloc_state_t *s, size_t size)
{
    void *ptr;

    /* Do not allocate zero bytes: behavior is platform dependent */
    assert(size != 0);

    if (unlikely(s->malloc_size + size > s->malloc_limit))
        return NULL;

    ptr = malloc(size);
    if (!ptr)
        return NULL;

    s->malloc_count++;
    s->malloc_size += qrk__def_malloc_usable_size(ptr) + MALLOC_OVERHEAD;

    qrk__generate_trace(s, ptr);

    return ptr;
}

static void qrk__def_free(qrk_malloc_state_t *s, void *ptr)
{
    if (!ptr)
        return;

    s->malloc_count--;
    s->malloc_size -= qrk__def_malloc_usable_size(ptr) + MALLOC_OVERHEAD;

    qrk__remove_trace(s, ptr);

    free(ptr);
}

static void *qrk__def_realloc(qrk_malloc_state_t *s, void *ptr, size_t size)
{
    size_t old_size;

    qrk__remove_trace(s, ptr);

    if (!ptr) {
        if (size == 0)
            return NULL;
        return qrk__def_malloc(s, size);
    }
    old_size = qrk__def_malloc_usable_size(ptr);
    if (size == 0) {
        s->malloc_count--;
        s->malloc_size -= old_size + MALLOC_OVERHEAD;
        free(ptr);
        return NULL;
    }
    if (s->malloc_size + size - old_size > s->malloc_limit)
        return NULL;

    ptr = realloc(ptr, size);
    if (!ptr)
        return NULL;

    s->malloc_size += qrk__def_malloc_usable_size(ptr) - old_size;

    qrk__generate_trace(s, ptr);

    return ptr;
}

static const qrk_malloc_funcs_t def_malloc_funcs = {
    (void *(*)(void *, size_t))qrk__def_malloc,
    (void (*)(void *, void *))qrk__def_free,
    (void *(*)(void *, void *, size_t))qrk__def_realloc,
#if defined(__APPLE__)
    malloc_size,
#elif defined(_WIN32)
    (size_t (*)(const void *))_msize,
#elif defined(EMSCRIPTEN)
    NULL,
#elif defined(__linux__)
    (size_t (*)(const void *))malloc_usable_size,
#else
    /* change this to `NULL,` if compilation fails */
    malloc_usable_size,
#endif
};

static void *qrk__unsafe_malloc(qrk_malloc_state_t *s, size_t size)
{
    return malloc(size);
}

static void qrk__unsafe_free(qrk_malloc_state_t *s, void *ptr)
{
    free(ptr);
}

static void *qrk__unsafe_realloc(qrk_malloc_state_t *s, void *ptr, size_t size)
{
    return realloc(ptr, size);
}

#include <quark/no_malloc.h>

qrk_malloc_ctx_t *qrk_malloc_ctx_unsafe()
{
    static qrk_malloc_ctx_t ctx = {
        .mf = {
            (void *(*)(void *, size_t))qrk__unsafe_malloc,
            (void (*)(void *, void *))qrk__unsafe_free,
            (void *(*)(void *, void *, size_t))qrk__unsafe_realloc,
            NULL
        }
    };

    return &ctx;
}

static size_t qrk__malloc_usable_size_unknown (const void *ptr)
{
    return 0;
}

void qrk_malloc_ctx_new_ex (qrk_malloc_ctx_t *ctx, const qrk_malloc_funcs_t *mf)
{
    memset(ctx, 0, sizeof(qrk_malloc_ctx_t));
    ctx->ms.malloc_limit = -1;

    ctx->mf = *mf;

    if (!ctx->mf.quark_malloc_usable_size)
        ctx->mf.quark_malloc_usable_size = qrk__malloc_usable_size_unknown;

#if defined(QRK_MALLOC_DEBUG) && defined(__linux__)
    if (qrk_hashmap_init(&ctx->ms.pointers, qrk_malloc_ctx_unsafe(), qrk_hashmap_hash_ptr, qrk_hashmap_compare_ptr, qrk_hashmap_dup_echo, qrk_hashmap_free_noop)) 
    {
        fputs("!!! Failed to create debug memory context\n", stderr);
        exit(1);
    }
#endif
}

void qrk_malloc_ctx_new (qrk_malloc_ctx_t *ctx)
{
    return qrk_malloc_ctx_new_ex(ctx, &def_malloc_funcs);
}

void qrk_malloc_ctx_set_limit (qrk_malloc_ctx_t *ctx, size_t limit)
{
    ctx->ms.malloc_limit = limit;
}

static void qrk__dump_leaks(const void *key, const void *value)
{
    puts((char *)value);
}

void qrk_malloc_ctx_dump_leaks (qrk_malloc_ctx_t *ctx)
{
    if (ctx->ms.malloc_count > 0)
        printf("Memory leak: %"PRIu64" bytes lost in %"PRIu64" block%c\n",
            (uint64_t)(ctx->ms.malloc_size),
            (uint64_t)(ctx->ms.malloc_count), ctx->ms.malloc_count > 1 ? 's' : ' ');
#if defined(QRK_MALLOC_DEBUG) && defined(__linux__)
    qrk_hashmap_iterate(&ctx->ms.pointers, qrk__dump_leaks);
#endif
}

void *qrk_malloc (qrk_malloc_ctx_t *ctx, size_t size) 
{
    return ctx->mf.quark_malloc(&ctx->ms, size);
}
void qrk_free (qrk_malloc_ctx_t *ctx, void *ptr)
{
    ctx->mf.quark_free(&ctx->ms, ptr);
}
void *qrk_realloc (qrk_malloc_ctx_t *ctx, void *ptr, size_t size)
{
    return ctx->mf.quark_realloc(&ctx->ms, ptr, size);
}
void *qrk_mallocz (qrk_malloc_ctx_t *ctx, size_t size)
{
    void *ptr;
    ptr = qrk_malloc(ctx, size);
    if (!ptr)
        return NULL;
    return memset(ptr, 0, size);
}
#if defined(QRK_MALLOC_DEBUG) && defined(__linux__)
#undef free
static void qrk__free_traces (const void *key, const void *value)
{
    free((void *)value);
}
void qrk_malloc_ctx_free(qrk_malloc_ctx_t *ctx)
{
    qrk_hashmap_iterate(&ctx->ms.pointers, qrk__free_traces);
    free(ctx->ms.pointers.table);
}
#endif