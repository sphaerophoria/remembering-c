#pragma once

#include <stddef.h>
#include <stdint.h>

#include "item_pool.h"

// Needs to create space for node_pool_buf, nodes, items_pool_buf, and buckets
//
// Declare 1 big aligned uint8 buffer that gets chopped up into the above 4 buffers
#define HASHMAP_BUF_SIZE(capacity) ITEM_POOL_BUF_SIZE(capacity) * 2 * sizeof(uint64_t) + sizeof(struct hashmap_node) * capacity + sizeof(struct hashmap_node*) * capacity

#define DECLARE_HASHMAP_BUFFERS(name, capacity) _Alignas(8) uint8_t name[HASHMAP_BUF_SIZE(capacity)]

struct hashmap {
    // Internal node allocations
    uint64_t *node_pool_buf;
    struct item_pool node_pool;
    struct hashmap_node *nodes; // Externally fed somehow

    // Actual hashmap buckets
    struct hashmap_node **buckets;

    // FIXME: num buckets is probably a multiplier on capacity
    size_t capacity;
};

struct hashmap hashmap_init(uint8_t *buffers, size_t capacity);

struct hashmap_node {
    uint64_t hash;
    struct hashmap_node *next; // null if end of list
};

struct hashmap_ref {
    // Public API
    size_t idx; // pointer to real element (duped from node)

    // Private
    uint64_t hash;

    struct hashmap_node *node;
    struct hashmap_node *prev;
    struct hashmap *map;
};

// FIXME: This macro needs some love
#define hashmap_get_impl(map, items, key, hash_fn, eql_fn) ({ \
    typeof(key) _hgi_key = (key); \
    struct hashmap_ref _hgi_it = hashmap_make_ref((map), hash_fn(_hgi_key)); \
    while (hashmap_ref_next(&_hgi_it)) { \
        if (eql_fn(_hgi_key, &items[_hgi_it.idx])) { \
            break; \
        } \
    } \
    _hgi_it; \
})

struct hashmap_ref hashmap_make_ref(struct hashmap *map, uint64_t hash);
bool hashmap_ref_next(struct hashmap_ref *ref);

int hashmap_insert(struct hashmap_ref *ref);
void hashmap_remove(struct hashmap_ref *ref);
