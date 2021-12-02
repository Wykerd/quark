#ifndef QRK_ALLOC_H
#define QRK_ALLOC_H

#include <quark/std/hashmap.h>

typedef struct qrk_malloc_state_s {
    size_t malloc_count;
    size_t malloc_size;
    size_t malloc_limit;
	void *opaque;
#if defined(QRK_MALLOC_DEBUG) && defined(__linux__)
    qrk_hashmap_t pointers;
#endif
} qrk_malloc_state_t;

typedef struct qrk_mallloc_funcs_s {
    void *(*quark_malloc)(void *state, size_t size);
    void (*quark_free)(void *state, void *ptr);
    void *(*quark_realloc)(void *state, void *ptr, size_t size);
    size_t (*quark_malloc_usable_size)(const void *ptr);
} qrk_malloc_funcs_t;

#define DEF_QRK_ALLOC_CONTEXT(state)   	\
    {                                   \
        qrk_malloc_funcs_t mf;          \
        state ms;                       \
    }

struct qrk_malloc_ctx_s 
    DEF_QRK_ALLOC_CONTEXT(qrk_malloc_state_t) 
;

#define QRK_MEMORY_CONTEXT_FIELDS		\
	qrk_malloc_ctx_t *m_ctx;

void qrk_malloc_ctx_new_ex (qrk_malloc_ctx_t *ctx, const qrk_malloc_funcs_t *mf);
void qrk_malloc_ctx_new (qrk_malloc_ctx_t *ctx);
void qrk_malloc_ctx_set_limit (qrk_malloc_ctx_t *ctx, size_t limit);
void qrk_malloc_ctx_dump_leaks (qrk_malloc_ctx_t *ctx);
void *qrk_malloc (qrk_malloc_ctx_t *ctx, size_t size);
void qrk_free (qrk_malloc_ctx_t *ctx, void *ptr);
void *qrk_realloc (qrk_malloc_ctx_t *ctx, void *ptr, size_t size);
#if defined(QRK_MALLOC_DEBUG) && defined(__linux__)
void qrk_malloc_ctx_free(qrk_malloc_ctx_t *ctx);
#else
#define qrk_malloc_ctx_free(ctx)
#endif
void *qrk_mallocz (qrk_malloc_ctx_t *ctx, size_t size);


#endif
