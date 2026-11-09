#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct item_pool {
    size_t num_elems;
    uint64_t* currently_allocated;
};

#define ITEM_POOL_BUF_SIZE(num_elems) ((num_elems + 63) / 64)
#define DECLARE_ITEM_POOL_BUF(name, num_elems) uint64_t name[ITEM_POOL_BUF_SIZE(num_elems)]

struct item_pool item_pool_init(uint64_t *buf, size_t num_elems);
int item_pool_alloc(struct item_pool *pool, size_t *out);
void item_pool_free(struct item_pool *pool, size_t idx);

struct item_pool_iter {
    size_t idx;
    struct item_pool *parent;
};

struct item_pool_iter item_pool_iter(struct item_pool *pool);
bool item_pool_iter_next(struct item_pool_iter *iter);
