#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "zhash.h"

static struct ZHashEntry *zcreate_entry(struct ZAllocator allocator, char *key, void *val);
static void zfree_entry(struct ZAllocator allocator, struct ZHashEntry *entry, bool recursive);
static size_t zgenerate_hash(struct ZHashTable *hash_table, char *key);
static bool zhash_rehash(struct ZHashTable *hash_table, size_t size_index);
static size_t znext_size_index(size_t size_index);
static size_t zprevious_size_index(size_t size_index);
static struct ZHashTable *zcreate_hash_table_with_size(struct ZAllocator allocator, size_t size_index);
static void *zcalloc(struct ZAllocator allocator, size_t num, size_t size);

// possible sizes for hash table; must be prime numbers
static const size_t hash_sizes[] = {
  53, 101, 211, 503, 1553, 3407, 6803, 12503, 25013, 50261,
  104729, 250007, 500009, 1000003, 2000029, 4000037, 10000019,
  25000009, 50000047, 104395301, 217645177, 512927357, 1000000007
};

// functions declared in zhash.h
struct ZHashTable *zcreate_hash_table(struct ZAllocator allocator)
{
  return zcreate_hash_table_with_size(allocator, 0);
}

void zfree_hash_table(struct ZHashTable *hash_table)
{
  size_t size, ii;

  size = hash_sizes[hash_table->size_index];

  for (ii = 0; ii < size; ii++) {
    struct ZHashEntry *entry;

    if ((entry = hash_table->entries[ii])) zfree_entry(hash_table->allocator, entry, true);
  }

  ZFreeFunc zfree = hash_table->allocator.free;
  zfree((void *) hash_table->entries);
  zfree((void *) hash_table);
}

bool zhash_set(struct ZHashTable *hash_table, char *key, void *val)
{
  size_t size, hash;
  struct ZHashEntry *entry;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  while (entry) {
    if (strcmp(key, entry->key) == 0) {
      entry->val = val;
      return true;
    }
    entry = entry->next;
  }

  entry = zcreate_entry(hash_table->allocator, key, val);
  if (!entry) { 
    return false;
  }

  entry->next = hash_table->entries[hash];
  hash_table->entries[hash] = entry;
  hash_table->entry_count++;

  size = hash_sizes[hash_table->size_index];

  if (hash_table->entry_count > size / 2) {
    return zhash_rehash(hash_table, znext_size_index(hash_table->size_index));
  }
  return true;
}

void *zhash_get(struct ZHashTable *hash_table, char *key)
{
  size_t hash;
  struct ZHashEntry *entry;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  while (entry && strcmp(key, entry->key) != 0) entry = entry->next;

  return entry ? entry->val : NULL;
}

void *zhash_delete(struct ZHashTable *hash_table, char *key)
{
  size_t size, hash;
  struct ZHashEntry *entry;
  void *val;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  if (entry && strcmp(key, entry->key) == 0) {
    hash_table->entries[hash] = entry->next;
  } else {
    while (entry) {
      if (entry->next && strcmp(key, entry->next->key) == 0) {
        struct ZHashEntry *deleted_entry;

        deleted_entry = entry->next;
        entry->next = entry->next->next;
        entry = deleted_entry;
        break;
      }
      entry = entry->next;
    }
  }

  if (!entry) return NULL;

  val = entry->val;
  zfree_entry(hash_table->allocator, entry, false);
  hash_table->entry_count--;

  size = hash_sizes[hash_table->size_index];

  if (hash_table->entry_count < size / 8) {
    zhash_rehash(hash_table, zprevious_size_index(hash_table->size_index));
  }

  return val;
}

bool zhash_exists(struct ZHashTable *hash_table, char *key)
{
  size_t hash;
  struct ZHashEntry *entry;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  while (entry && strcmp(key, entry->key) != 0) entry = entry->next;

  return entry ? true : false;
}

// helper functions, definitions
static struct ZHashTable *zcreate_hash_table_with_size(struct ZAllocator allocator, size_t size_index)
{
  struct ZHashTable *hash_table;

  hash_table = (struct ZHashTable *) allocator.alloc(sizeof(struct ZHashTable));
  if (!hash_table) {
    return NULL;
  }

  hash_table->size_index = size_index;
  hash_table->entry_count = 0;
  hash_table->entries = zcalloc(allocator, hash_sizes[size_index], sizeof(void *));
  if (!hash_table->entries) {
    allocator.free(hash_table);
    return NULL;
  }
  hash_table->allocator = allocator;

  return hash_table;
}

static struct ZHashEntry *zcreate_entry(struct ZAllocator allocator, char *key, void *val)
{
  struct ZHashEntry *entry;
  char *key_cpy;

  key_cpy = (char *) allocator.alloc((strlen(key) + 1) * sizeof(char));
  if (!key_cpy) {
    return NULL;
  }
  entry = (struct ZHashEntry *) allocator.alloc(sizeof(struct ZHashEntry));
  if (!entry) {
    allocator.free(key_cpy);
    return NULL;
  }

  strcpy(key_cpy, key);
  entry->key = key_cpy;
  entry->val = val;

  return entry;
}

static void zfree_entry(struct ZAllocator allocator, struct ZHashEntry *entry, bool recursive)
{
  struct ZHashEntry *next;

  if (!recursive) entry->next = NULL;

  const ZFreeFunc zfree = allocator.free;

  while (entry) {
    next = entry->next;

    zfree((void *) entry->key);
    zfree((void *) entry);

    entry = next;
  }
}

static size_t zgenerate_hash(struct ZHashTable *hash_table, char *key)
{
  size_t size, hash;
  char ch;

  size = hash_sizes[hash_table->size_index];
  hash = 0;

  while ((ch = *key++)) hash = (17 * hash + ch) % size;

  return hash;
}

static bool zhash_rehash(struct ZHashTable *hash_table, size_t size_index)
{
  size_t hash, size, ii;
  struct ZHashEntry **entries;

  if (size_index == hash_table->size_index) return true;

  size = hash_sizes[hash_table->size_index];
  entries = hash_table->entries;

  hash_table->size_index = size_index;
  hash_table->entries = zcalloc(hash_table->allocator, hash_sizes[size_index], sizeof(void *));
  if (!hash_table->entries) {
    return false;
  }

  for (ii = 0; ii < size; ii++) {
    struct ZHashEntry *entry;

    entry = entries[ii];
    while (entry) {
      struct ZHashEntry *next_entry;

      hash = zgenerate_hash(hash_table, entry->key);
      next_entry = entry->next;
      entry->next = hash_table->entries[hash];
      hash_table->entries[hash] = entry;

      entry = next_entry;
    }
  }

  hash_table->allocator.free((void *) entries);
  return true;
}

static size_t znext_size_index(size_t size_index)
{
  if (size_index == ZCOUNT_OF(hash_sizes)) return size_index;

  return size_index + 1;
}

static size_t zprevious_size_index(size_t size_index)
{
  if (size_index == 0) return size_index;

  return size_index - 1;
}


static void *zcalloc(struct ZAllocator allocator, size_t num, size_t size)
{
  void *ptr = allocator.alloc(num * size);
  if (ptr) {
    memset(ptr, 0, num * size);
  }

  return ptr;
}
