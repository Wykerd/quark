#ifndef QRK_STD_HASHMAP_H
#define QRK_STD_HASHMAP_H

#include <inttypes.h>
#include <stddef.h>

typedef struct qrk_malloc_ctx_s qrk_malloc_ctx_t;

/**
 * Hashmap
 */

/* Must be a power of two */
#define QRK_HASHMAP_DEFAULT_SIZE   4 // 2**7
/* Double table size and rehash when load factor greater than equal to this value */
#define QRK_MAXIMUM_LOAD_FACTOR    0.7

typedef int (*qrk_hashmap_cmp)(const void *, const void *);
typedef uint64_t (*qrk_hashmap_hash)(const void *);
typedef void *(*qrk_hashmap_key_dup)(qrk_malloc_ctx_t *m_ctx, const void *);
typedef void (*qrk_hashmap_key_free)(qrk_malloc_ctx_t *m_ctx, void *);
typedef void (*qrk_hashmap_iterator)(const void *key, const void *value);

typedef struct qrk_hashmap_entry_s {
    uint64_t hash;
    void *key;
    void *value;
} qrk_hashmap_entry_t;

typedef struct qrk_hashmap_s {
    qrk_hashmap_entry_t *table;
    size_t len;
    size_t size;

    qrk_malloc_ctx_t *m_ctx;
    qrk_hashmap_cmp compare;
    qrk_hashmap_hash hash;
    qrk_hashmap_key_dup dup_key;
    qrk_hashmap_key_free free_key;
} qrk_hashmap_t;

double qrk_hashmap_load_factor (qrk_hashmap_t *map);
uint64_t qrk_hashmap_hash_str (const void *str);
uint64_t qrk_hashmap_hash_ptr (const void *ptr);
int qrk_hashmap_compare_ptr (const void *ptr1, const void *ptr2);
void *qrk_hashmap_dup_echo (qrk_malloc_ctx_t *m_ctx, const void *str);
void qrk_hashmap_free_noop (qrk_malloc_ctx_t *m_ctx, void *str);
void *qrk_hashmap_dup_str (qrk_malloc_ctx_t *m_ctx, const void *str);
void qrk_hashmap_free_str (qrk_malloc_ctx_t *m_ctx, void *str);
int qrk_hashmap_init_str (qrk_hashmap_t *map, qrk_malloc_ctx_t *m_ctx);
int qrk_hashmap_init (qrk_hashmap_t *map, qrk_malloc_ctx_t *m_ctx, qrk_hashmap_hash hash_func, qrk_hashmap_cmp cmp_func, qrk_hashmap_key_dup dup_key, qrk_hashmap_key_free free_key);
int qrk_hashmap_set (qrk_hashmap_t *map, const void *key, void *value);
int qrk_hashmap_replace (qrk_hashmap_t *map, const void *key, void *value);
void *qrk_hashmap_delete (qrk_hashmap_t *map, const void *key);
void *qrk_hashmap_get (qrk_hashmap_t *map, const void *key);
void qrk_hashmap_iterate (qrk_hashmap_t *map, qrk_hashmap_iterator iterator);
void qrk_hashmap_free (qrk_hashmap_t *map);

#endif
