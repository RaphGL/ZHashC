#ifndef ZHASH_H
#define ZHASH_H

#include <stdbool.h>
#include <stdlib.h>

// hash table
// keys are strings
// values are void *pointers

#define ZCOUNT_OF(arr) (sizeof(arr) / sizeof(*arr))

typedef void *(*ZMallocFunc)(size_t);
typedef void (*ZFreeFunc)(void *);

struct ZAllocator {
  ZMallocFunc alloc;
  ZFreeFunc free;
};

#define ZDEFAULT_ALLOCATOR ((struct ZAllocator) { .alloc = malloc, .free = free })

// struct representing an entry in the hash table
struct ZHashEntry {
  char *key;
  void *val;
  struct ZHashEntry *next;
};

// struct representing the hash table
// size_index is an index into the hash_sizes array in hash.c
struct ZHashTable {
  size_t size_index;
  size_t entry_count;
  struct ZAllocator allocator;
  struct ZHashEntry **entries;
};

// hash table creation and destruction
struct ZHashTable *zcreate_hash_table(void);
struct ZHashTable *zcreate_hash_table_with_allocator(struct ZAllocator);
void zfree_hash_table(struct ZHashTable *hash_table);

// hash table operations
bool zhash_set(struct ZHashTable *hash_table, char *key, void *val);
void *zhash_get(struct ZHashTable *hash_table, char *key);
void *zhash_delete(struct ZHashTable *hash_table, char *key);
bool zhash_exists(struct ZHashTable *hash_table, char *key);

#endif
