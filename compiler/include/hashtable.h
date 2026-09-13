#pragma once

#include <stdint.h>
#include <stddef.h>

#define HT_INITIAL_CAPACITY 32

typedef struct
{
    struct _hashtable_entry *entries;
    size_t capacity;

    size_t count;
    size_t tombstones;
} hashtable_t;

typedef enum
{
    HT_ENTRY_FREE = 0,
    HT_ENTRY_USED = 1,
    HT_ENTRY_TOMBSTONE = 2,
} hashtable_entry_state_t;

typedef struct _hashtable_entry
{
    uint64_t key_hash;
    void *value;

    hashtable_entry_state_t state;
} hashtable_entry_t;

uint64_t hash_key_str(const char *key);
uint64_t hash_key_uint64_t(uint64_t key);

void hashtable_create(hashtable_t *table);
void hashtable_free(hashtable_t *table);
void hashtable_extend(hashtable_t *table);
void hashtable_insert(hashtable_t *table, uint64_t key_hash, void *value);
void *hashtable_get(hashtable_t *table, uint64_t key_hash);
void hashtable_remove(hashtable_t *table, uint64_t key_hash);

#define ht_insert(table, key, value)                                                                                             \
    hashtable_insert(table, _Generic((key), uint64_t: hash_key_uint64_t, char *: hash_key_str, const char *: hash_key_str)(key), \
                     value)

#define ht_get(table, key) \
    hashtable_get(table, _Generic((key), uint64_t: hash_key_uint64_t, char *: hash_key_str, const char *: hash_key_str)(key))

#define ht_remove(table, key) \
    hashtable_remove(table, _Generic((key), uint64_t: hash_key_uint64_t, char *: hash_key_str, const char *: hash_key_str)(key))

#define ht_foreach(table, it) for (size_t i = 0; i < table->capacity; it = &table->entries[i++])
