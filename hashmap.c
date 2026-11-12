#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include "hashmap.h"


// FIXME: unduplicate with memory.h
static inline void* align_forward(void* ptr, size_t alignment) {
    // (x + page_size - 1) & ~(page_size - 1) for align up
    size_t ptr_int = (size_t)ptr;
    size_t mask = alignment - 1;
    ptr_int += mask;
    ptr_int &= ~mask;
    return (void*)ptr_int;
}

static size_t node_idx_from_node(struct hashmap_node* nodes, struct hashmap_node *node) {
    if (node == NULL) return SIZE_MAX;
    return (size_t)(node - nodes);
}

struct hashmap hashmap_init(uint8_t *buffers, size_t capacity) {
    assert((size_t)buffers % alignof(uint64_t) == 0);

    uint64_t *node_pool_buf = (uint64_t*)buffers;
    buffers += ITEM_POOL_BUF_SIZE(capacity) * sizeof(uint64_t);

    //buffers = align_forward(buffers, alignof(struct hashmap_node));
    struct hashmap_node *nodes = (struct hashmap_node*)buffers;
    buffers += capacity * sizeof(struct hashmap_node);

    //buffers = align_forward(buffers, alignof(struct hashmap_node*));
    struct hashmap_node **buckets = (struct hashmap_node**)buffers;
    for (size_t i = 0; i < capacity; i++) {
        buckets[i] = NULL;
    }

    return (struct hashmap) {
        .node_pool_buf = node_pool_buf,
        .node_pool = item_pool_init(node_pool_buf, capacity),
        .nodes = nodes,

        .buckets = buckets,
        .capacity = capacity,
    };
}


struct hashmap_ref hashmap_make_ref(struct hashmap *map, uint64_t hash) {
    return (struct hashmap_ref) {
        .idx = SIZE_MAX,
        .hash = hash,
        .node = NULL,
        .prev = NULL,
        .map = map,
    };
}

bool hashmap_ref_next(struct hashmap_ref *ref) {
    while (true) {
        if (ref->idx == SIZE_MAX && ref->node == NULL && ref->prev == NULL) {
            // FIXME: At some point we should only use the first bit of the buffer so that we can unmap the back half if the map is really large
            size_t bucket_idx = ref->hash % ref->map->capacity;
            struct hashmap_node *head = ref->map->buckets[bucket_idx];

            ref->node = head;
            ref->prev = NULL;
            ref->idx = node_idx_from_node(ref->map->nodes, ref->node);
        } else if (ref->idx == SIZE_MAX) {
            return false;
        } else {
            ref->prev = ref->node;
            ref->node = ref->node->next;
            ref->idx = node_idx_from_node(ref->map->nodes, ref->node);
        }

        if (ref->node && ref->hash != ref->node->hash) {
            continue;
        }

        return ref->idx != SIZE_MAX;
    }
}


int hashmap_insert(struct hashmap_ref *ref) {
    if (ref->idx != SIZE_MAX) return -EEXIST;

    int ret = 0;
    size_t node_idx;
    if ((ret = item_pool_alloc(&ref->map->node_pool, &node_idx))) return ret;

    ref->map->nodes[node_idx] = (struct hashmap_node) {
        .hash = ref->hash,
        .next = NULL,
    };

    if (ref->prev) {
        ref->prev->next = &ref->map->nodes[node_idx];
    } else {
        // FIXME: duplicated
        size_t bucket_idx = ref->hash % ref->map->capacity;
        ref->map->buckets[bucket_idx] = &ref->map->nodes[node_idx];
    }
    ref->node = &ref->map->nodes[node_idx];

    ref->idx = node_idx;

    return ret;
}

void hashmap_remove(struct hashmap_ref *ref) {
    if (ref->idx == SIZE_MAX) return;

    if (ref->prev) {
        ref->prev->next = ref->node->next;
    } else {
        // FIXME: duplicated
        size_t bucket_idx = ref->hash % ref->map->capacity;
        ref->map->buckets[bucket_idx] = ref->node->next;
    }

    assert(ref->node != NULL);
    item_pool_free(&ref->map->node_pool, node_idx_from_node(ref->map->nodes, ref->node));
}
