#include <quark/std/hashmap.h>
#include <quark/std/alloc.h>
#include <quark/std/spooky.h>
#include <string.h>
#include <errno.h>

#include <quark/no_malloc.h>

double qrk_hashmap_load_factor (qrk_hashmap_t *map) 
{
    return (map->len * 1.0) / (map->size * 1.0);
};

uint64_t qrk_hashmap_hash_str (const void *str)
{
    return qrk_spooky_hash64(str, strlen(str), 0);
}

uint64_t qrk_hashmap_hash_ptr (const void *ptr)
{
    return qrk_spooky_hash64(ptr, sizeof(ptr), 0);
}

void *qrk_hashmap_dup_str (qrk_malloc_ctx_t *m_ctx, const void *str)
{
    size_t len = strlen(str);
    char *n_str = qrk_malloc(m_ctx, len + 1);
    if (!n_str)
        return NULL;
    memcpy(n_str, str, len);
    n_str[len] = '\0';
    return n_str;
}

void *qrk_hashmap_dup_echo (qrk_malloc_ctx_t *m_ctx, const void *str)
{
    return (void *)str;
}

void qrk_hashmap_free_noop (qrk_malloc_ctx_t *m_ctx, void *str)
{
    /* noop */
}

void qrk_hashmap_free_str (qrk_malloc_ctx_t *m_ctx, void *str)
{
    qrk_free(m_ctx, str);
}

int qrk_hashmap_compare_ptr (const void *ptr1, const void *ptr2)
{
    return !(ptr1 == ptr2);
}

int qrk_hashmap_init_str (qrk_hashmap_t *map, qrk_malloc_ctx_t *m_ctx)
{
    return qrk_hashmap_init(map, m_ctx, qrk_hashmap_hash_str, (int (*)(const void *, const void *))strcmp, qrk_hashmap_dup_str, qrk_hashmap_free_str);
}

int qrk_hashmap_init (qrk_hashmap_t *map, qrk_malloc_ctx_t *m_ctx, qrk_hashmap_hash hash_func, qrk_hashmap_cmp cmp_func, qrk_hashmap_key_dup dup_key, qrk_hashmap_key_free free_key)
{
    memset(map, 0, sizeof(qrk_hashmap_t));
    map->m_ctx = m_ctx;
    map->hash = hash_func;
    map->compare = cmp_func;
    map->dup_key = dup_key;
    map->free_key = free_key;
    map->size = QRK_HASHMAP_DEFAULT_SIZE;
    map->len = 0;
    map->table = qrk_mallocz(map->m_ctx, sizeof(qrk_hashmap_entry_t) * QRK_HASHMAP_DEFAULT_SIZE);
    if (!map->table)
        return ENOMEM;
    return 0;
}

/**
 * @param entry Set ot the memory location of the entry or the next open location for insertion
 * @returns The index of the entry or the next open location for insertion 
 */
static inline
size_t qrk__hashmap_find (qrk_hashmap_t *map, uint64_t hash, const void *key, qrk_hashmap_entry_t **entry)
{
    size_t idx = hash & (map->size - 1);

    while (map->table[idx].key)
    {
        if (!map->compare(key, map->table[idx].key))
            break;
        idx = (idx + 1) & (map->size - 1);
    }

    *entry = &map->table[idx];
    return idx;
}

static inline
int qrk__hashmap_grow (qrk_hashmap_t *map)
{
    qrk_hashmap_entry_t *old_table = map->table;
    size_t old_size = map->size;

    qrk_hashmap_entry_t *new_table = qrk_mallocz(map->m_ctx, sizeof(qrk_hashmap_entry_t) * (map->size * 2));
    if (!new_table)
        return ENOMEM;
    
    map->table = new_table;
    map->size *= 2;

    for (size_t i = 0; i < old_size; i++)
        if (old_table[i].key)
        {
            qrk_hashmap_entry_t *entry;
            qrk__hashmap_find(map, old_table[i].hash, old_table[i].key, &entry);

            entry->key = old_table[i].key;
            entry->value = old_table[i].value;
        };

    qrk_free(map->m_ctx, old_table);

    return 0;
}

int qrk_hashmap_set (qrk_hashmap_t *map, const void *key, void *value)
{
    qrk_hashmap_entry_t *entry;
    uint64_t hash = map->hash(key);
    qrk__hashmap_find(map, hash, key, &entry);

    if (entry->key)
        return EEXIST;

    entry->key = map->dup_key(map->m_ctx, key);
    if (!entry->key)
        return ENOMEM;

    entry->value = value;
    entry->hash = hash;

    map->len++;

    if (qrk_hashmap_load_factor(map) >= QRK_MAXIMUM_LOAD_FACTOR)
        return qrk__hashmap_grow(map);
        
    return 0;
}


int qrk_hashmap_replace (qrk_hashmap_t *map, const void *key, void *value)
{
    qrk_hashmap_entry_t *entry;
    uint64_t hash = map->hash(key);
    qrk__hashmap_find(map, hash, key, &entry);

    if (!entry->key)
    {
        entry->key = map->dup_key(map->m_ctx, key);
        if (!entry->key)
            return ENOMEM;
        
        map->len++;
    }

    entry->value = value;
    entry->hash = hash;

    if (qrk_hashmap_load_factor(map) >= QRK_MAXIMUM_LOAD_FACTOR)
        return qrk__hashmap_grow(map);
        
    return 0;
}

void *qrk_hashmap_delete (qrk_hashmap_t *map, const void *key)
{
    qrk_hashmap_entry_t *entry;
    uint64_t hash = map->hash(key);
    size_t idx = qrk__hashmap_find(map, hash, key, &entry);

    if (!entry->key)
        return NULL;

    void *value = entry->value;

    map->free_key(map->m_ctx, entry->key);

    map->len--;

    size_t search_idx = (idx + 1) & (map->size - 1);

    for (size_t i = 0; i < map->size; i++)
    {
        if (!map->table[search_idx].key) /* Empty cell */
            break;

        /* If the new one has a lower or equal index */
        if ((map->table[search_idx].hash & (map->size - 1)) <= idx)
        {
            *entry = map->table[search_idx]; /* Replace the old entry with the new one */
            entry = &map->table[search_idx]; /* Set the entry to the new one */
            idx = map->table[search_idx].hash & (map->size - 1); /* Now find breaks in the chain for new one */
        }

        search_idx = (search_idx + 1) & (map->size - 1);
    }

    memset(entry, 0, sizeof(qrk_hashmap_entry_t));

    return value;
}

void *qrk_hashmap_get (qrk_hashmap_t *map, const void *key)
{
    qrk_hashmap_entry_t *entry;
    uint64_t hash = map->hash(key);
    qrk__hashmap_find(map, hash, key, &entry);

    if (entry->key)
        return entry->value;

    return NULL;
}

void qrk_hashmap_iterate (qrk_hashmap_t *map, qrk_hashmap_iterator iterator)
{
    for (size_t i = 0; i < map->size; i++)
        if (map->table[i].key)
            iterator(map->table[i].key, map->table[i].value);
}

void qrk_hashmap_free (qrk_hashmap_t *map)
{
    for (size_t i = 0; i < map->size; i++)
        if (map->table[i].key)
            map->free_key(map->m_ctx, map->table[i].key);

    qrk_free(map->m_ctx, map->table);

    map->size = 0;
    map->len = 0;
}
