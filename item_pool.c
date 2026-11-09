#include "item_pool.h"

#include <string.h>
#include <assert.h>
#include <errno.h>

struct item_pool item_pool_init(uint64_t *buf, size_t num_elems) {
    memset(buf, 0, sizeof(uint64_t) * ITEM_POOL_BUF_SIZE(num_elems));

    return (struct item_pool) {
        .num_elems = num_elems,
        .currently_allocated = buf,
    };
}

int item_pool_alloc(struct item_pool *pool, size_t *out) {
    for (size_t i = 0; i < ITEM_POOL_BUF_SIZE(pool->num_elems); i++) {
        if (pool->currently_allocated[i] == UINT64_MAX) continue;
        int first_free = __builtin_ctzl(~pool->currently_allocated[i]);

        *out = i * 64 + first_free;

        if (*out >= pool->num_elems) {
            return -ENOMEM;
        }

        assert((pool->currently_allocated[i] & (size_t)1 << first_free) == 0);
        pool->currently_allocated[i] |= (size_t)1 << first_free;
        return 0;
    }

    return -ENOMEM;
}

void item_pool_free(struct item_pool *pool, size_t idx) {
    size_t outer = idx / 64;
    size_t inner = (size_t)1 << (idx % 64);
    assert((pool->currently_allocated[outer] & inner) != 0);
    pool->currently_allocated[outer] &= ~inner;
}

struct item_pool_iter item_pool_iter(struct item_pool *pool) {
    return (struct item_pool_iter) {
        .idx = SIZE_MAX,
        .parent = pool,
    };
}

bool item_pool_iter_next(struct item_pool_iter *it) {
    struct item_pool *pool = it->parent;
    while (true) {
        it->idx += 1;
        if (it->idx >= pool->num_elems) return false;

        size_t outer = it->idx / 64;
        size_t inner = (size_t)1 << (it->idx % 64);

        if ((pool->currently_allocated[outer] & inner) != 0) {
            return true;
        }
    }
}
