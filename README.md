# Hash Table

This is a hash table library implemented in C. Both a basic hash table (ZHash)
and a hash table with entries sorted by insertion order (ZSortedHash) are
provided. The keys are strings and the values are void pointers.

The following hashing algorithm is used:
```c
hash = 0;
while ((ch = *key++)) hash = (17 * hash + ch) % size;
```
(See
[http://algs4.cs.princeton.edu/34hash/](http://algs4.cs.princeton.edu/34hash/)
for more information about hash functions.)

Collisions are resolved with separate chaining and a singly linked list.
If the hash table is more than 50% full, it will increase the number of slots
and rehash. Likewise, if it's less than 12.5% full, it will decrease the number
of slots and rehash.

The possible numbers of slots are all prime numbers; each size is roughly two
times the previous size. The maximum number of slots is `1000000007` so
performance may degrade with more than  `1000000007 / 2` entries.

## ZHash

Standard hash table. Basic hash table operations are supported: `set`, `get`,
`exists`, `delete`.

### Example

```c
#include <stdio.h>
#include "../src/zhash.h"

// gcc -Wall -Wextra hello.c ../src/zhash.c
// prints "hello world" to stdout
int main ()
{
  struct ZHashTable *hash_table = zcreate_hash_table(ZDEFAULT_ALLOCATOR);

  zhash_set(hash_table, "hello", (void *) "world");

  if (zhash_exists(hash_table, "hello")) {
    printf("hello %s\n", (char *) zhash_get(hash_table, "hello"));
  }

  if (zhash_exists(hash_table, "goodbye")) {
    printf("goodbye %s\n", (char *) zhash_get(hash_table, "goodbye"));
  }

  zfree_hash_table(hash_table);

  return 0;
}
```

### Public Interface

```c
// create hash table
struct ZHashTable *zcreate_hash_table(void);

// create a hash table with a custom allocator
struct ZHashTable *zcreate_hash_table_with_allocator(struct ZAllocator);

// free hash table (note that this only frees the table and the entry structs)
void zfree_hash_table(struct ZHashTable *hash_table);

// set key to val (if there is already a value, overwrite it)
bool zhash_set(struct ZHashTable *hash_table, char *key, void *val);

// get the value stored at key (if no value, return NULL)
void *zhash_get(struct ZHashTable *hash_table, char *key);

// delete entry stored at key and return the value (if no value, return NULL)
void *zhash_delete(struct ZHashTable *hash_table, char *key);

// return true if there is a value stored at the key and false otherwise
bool zhash_exists(struct ZHashTable *hash_table, char *key);
```

## ZSortedHash

Hash table with entries sorted by insertion order. The same hash table
operations as ZHash are supported. In addition, an iterator is provided, which
can be used to iterate through entries.

ZSortedHash is built on top of ZHash. Essentially, what it's doing is wrapping
the the value in a linked list node when it is inserted. Then it does the
necessary bookkeeping when an entry is deleted.

### Example

```c
#include <stdio.h>
#include "../src/zsorted_hash.h"

// gcc -Wall -Wextra sorted_hello.c ../src/zhash.c ../src/zsorted_hash.c
// prints "hello world" in English and French to stdout
int main ()
{
  struct ZSortedHashTable *hash_table = zcreate_sorted_hash_table(ZDEFAULT_ALLOCATOR);

  zsorted_hash_set(hash_table, "hello", (void *) "world");
  zsorted_hash_set(hash_table, "bonjour", (void *) "le monde");

  for (struct ZIterator *iterator = zcreate_iterator(ZDEFAULT_ALLOCATOR, hash_table);
      ziterator_exists(iterator); ziterator_next(iterator)) {

    printf("%s %s\n", ziterator_get_key(iterator),
        (char *) ziterator_get_val(iterator));
  }

  zfree_iterator(iterator);
  zfree_sorted_hash_table(hash_table);

  return 0;
}
```

### Custom Allocators

Custom allocators are a changing the allocation strategy in a program. Often when doing allocations all objects being allocated share the same lifetime,
meaning that these values will either be deallocated together or not too far apart from each other. In these situations being able to allocate them normally but 
being able to discard all of them at once makes managing memory much easier. There's multiple types of allocators out there, if you want to learn more about them
I recommend checking out [gingerBill's series on custom allocators](https://www.gingerbill.org/series/memory-allocation-strategies/).

The basic usage of custom allocators in zhash is by passing along the malloc and free functions for your allocator with the `struct ZAllocator` type:
```c
struct ZAllocator {
  ZMallocFunc alloc;
  ZFreeFunc free;
};
```
If you're not using a general purpose allocator and you have some sort of `free_all` function that you run at a specific point in your code,
you can opt for having a no-op free function like this:
```c
void free_noop(void *buf) { return; }
```
Then you don't have to worry about the hash table freeing memory.


 ### Public Interface

```c
// these functions behave the same as their counterparts in zhash.h
struct ZSortedHashTable *zcreate_sorted_hash_table(void);
struct ZSortedHashTable *zcreate_sorted_hash_table_with_allocator(struct ZAllocator allocator);
void zfree_sorted_hash_table(struct ZSortedHashTable *hash_table);
bool zsorted_hash_set(struct ZSortedHashTable *hash_table, char *key, void *val);
void *zsorted_hash_get(struct ZSortedHashTable *hash_table, char *key);
void *zsorted_hash_delete(struct ZSortedHashTable *hash_table, char *key);
bool zsorted_hash_exists(struct ZSortedHashTable *hash_table, char *key);

// create an iterator to be used in iteration functions below
struct ZIterator *zcreate_iterator(struct ZSortedHashTable *hash_table);

// create an iterator with a custom allocator
struct ZIterator *zcreate_iterator_with_allocator(struct ZAllocator allocator, struct ZSortedHashTable *hash_table);

// free iterator
void zfree_iterator(struct ZIterator *iterator);

// return number of entries stored in the hash table
size_t zsorted_hash_count(struct ZSortedHashTable *hash_table);

// return true if the iterator is within bounds and false otherwise
bool ziterator_exists(struct ZIterator *iterator);

// get the key at the current position (if out of bounds, then return NULL)
char *ziterator_get_key(struct ZIterator *iterator);

// get the value at the current position (if out of bounds, then return NULL)
void *ziterator_get_val(struct ZIterator *iterator);

// move the position forward by one step
void ziterator_next(struct ZIterator *iterator);

// move the position back by one step
void ziterator_prev(struct ZIterator *iterator);
```

## Running Tests

```bash
cd ./test
./zhash_test.sh
```
The test uses either the `valgrind` or `leaks` command to detect memory leaks.
The tests will fail if neither is present.

## Notes

Tested on `Ubuntu 22.04` with `GCC 11.3.0`,
tested on `CachyOS` with `GCC 16.1.1`
and on `macOS 13.3.1` with `Apple clang version 14.0.3`.
