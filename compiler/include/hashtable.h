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
    char *key;
    void *value;

    hashtable_entry_state_t state;
} hashtable_entry_t;

void hashtable_create(hashtable_t *table);
void hashtable_free(hashtable_t *table);
void hashtable_extend(hashtable_t *table);
void hashtable_insert(hashtable_t *table, const char *key, void *value);
void *hashtable_get(hashtable_t *table, const char *key);
void hashtable_remove(hashtable_t *table, const char *key);

#define ht_foreach(table, it) for (size_t i = 0; i < table->capacity; it = &table->entries[i++])
