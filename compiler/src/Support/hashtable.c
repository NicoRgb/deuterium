#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hashtable.h"
#include "logger.h"

#define FNV_OFFSET UINT64_C(14695981039346656037)
#define FNV_PRIME UINT64_C(1099511628211)

static uint64_t hash_key(const char *key)
{
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++)
    {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

static void hashtable_create_with_capacity(hashtable_t *table, size_t capacity)
{
    ASSERT(table);

    table->capacity = capacity;
    table->count = 0;
    table->tombstones = 0;

    table->entries = malloc(sizeof(hashtable_entry_t) * table->capacity);
    if (!table->entries)
    {
        log_error("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    memset(table->entries, 0, sizeof(hashtable_entry_t) * table->capacity);
}

void hashtable_create(hashtable_t *table)
{
    hashtable_create_with_capacity(table, HT_INITIAL_CAPACITY);
}

void hashtable_free(hashtable_t *table)
{
    ASSERT(table);
    ASSERT(table->entries);

    hashtable_entry_t *entry;
    ht_foreach(table, entry)
    {
        if (entry->key == NULL)
            continue;

        free(entry->key);
    }

    free(table->entries);

    table->entries = NULL;
    table->capacity = 0;
    table->count = 0;
    table->tombstones = 0;
}

void hashtable_extend(hashtable_t *table)
{
    ASSERT(table);

    hashtable_t new;
    hashtable_create_with_capacity(&new, table->capacity * 2);

    hashtable_entry_t *entry;
    ht_foreach(table, entry)
    {
        if (entry->key == NULL)
            continue;

        hashtable_insert(&new, entry->key, entry->value);
        free(entry->key); // strduped in hashtable_insert
    }

    free(table->entries);
    table->capacity = new.capacity;
    table->entries = new.entries;
}

void hashtable_insert(hashtable_t *table, const char *key, void *value)
{
    ASSERT(table);
    ASSERT(key);

    if ((table->count + table->tombstones + 1) * 3 >= table->capacity * 2)
        hashtable_extend(table);

    uint64_t hash = hash_key(key);
    size_t index = hash % table->capacity;

    hashtable_entry_t *entry = &table->entries[index];
    size_t tombstone = SIZE_MAX;
    while (1)
    {
        if (entry->state == HT_ENTRY_FREE)
        {
            if (tombstone != SIZE_MAX)
                entry = &table->entries[tombstone];

            break;
        }

        if (entry->state == HT_ENTRY_TOMBSTONE && tombstone == SIZE_MAX)
        {
            tombstone = index;
        }
        else if (entry->state == HT_ENTRY_USED && strcmp(entry->key, key) == 0)
        {
            entry->value = value;
            return;
        }

        index = (index + 1) % table->capacity;
        entry = &table->entries[index];
    }

    table->count++;
    if (entry->state == HT_ENTRY_TOMBSTONE)
        table->tombstones--;

    entry->key = strdup(key);
    entry->value = value;
    entry->state = HT_ENTRY_USED;
}

static hashtable_entry_t *hashtable_get_entry(hashtable_t *table, const char *key)
{
    ASSERT(table);
    ASSERT(key);

    uint64_t hash = hash_key(key);
    size_t index = hash % table->capacity;
    size_t initial_index = index;

    hashtable_entry_t *entry = &table->entries[index];
    while (entry->state == HT_ENTRY_USED || entry->state == HT_ENTRY_TOMBSTONE)
    {
        if (entry->state == HT_ENTRY_USED && strcmp(entry->key, key) == 0)
            return entry;

        index = (index + 1) % table->capacity;
        entry = &table->entries[index];

        if (index == initial_index)
            return NULL;
    }

    return NULL;
}

void *hashtable_get(hashtable_t *table, const char *key)
{
    hashtable_entry_t *entry = hashtable_get_entry(table, key);
    if (!entry)
        return NULL;

    return entry->value;
}

void hashtable_remove(hashtable_t *table, const char *key)
{
    hashtable_entry_t *entry = hashtable_get_entry(table, key);
    if (!entry)
        return;

    entry->key = NULL;
    entry->state = HT_ENTRY_TOMBSTONE;

    table->count--;
    table->tombstones++;
}
