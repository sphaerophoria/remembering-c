#include "tests.h"
#include "item_pool.h"
#include <errno.h>

static int item_pool_buf_sizing(void) {
    if (ITEM_POOL_BUF_SIZE(0) != 0) return -1;
    if (ITEM_POOL_BUF_SIZE(1) != 1) return -1;
    if (ITEM_POOL_BUF_SIZE(64) != 1) return -1;
    return 0;
}

static int item_pool_sanity(void) {
    DECLARE_ITEM_POOL_BUF(item_pool_buf, 2);

    struct item_pool pool = item_pool_init(item_pool_buf, 2);

    size_t first;
    if (item_pool_alloc(&pool, &first)) return -1;

    size_t second;
    if (item_pool_alloc(&pool, &second)) return -1;

    size_t third;
    if (item_pool_alloc(&pool, &third) != -ENOMEM) return -1;

    item_pool_free(&pool, second);

    size_t fourth;
    if (item_pool_alloc(&pool, &fourth)) return -1;
    if (fourth != second) return -1;

    return 0;
}

static int item_pool_iter_sanity(void) {
    DECLARE_ITEM_POOL_BUF(item_pool_buf, 7);
    struct item_pool pool = item_pool_init(item_pool_buf, 7);

    {
        struct item_pool_iter it = item_pool_iter(&pool);
        if (item_pool_iter_next(&it)) return -1;
    }

    pool.currently_allocated[0] = 0x71;

    {
        struct item_pool_iter it = item_pool_iter(&pool);
        if (!item_pool_iter_next(&it)) return -1;
        if (it.idx != 0) return -1;
        if (!item_pool_iter_next(&it)) return -1;
        if (it.idx != 4) return -1;
        if (!item_pool_iter_next(&it)) return -1;
        if (it.idx != 5) return -1;
        if (!item_pool_iter_next(&it)) return -1;
        if (it.idx != 6) return -1;
        if (item_pool_iter_next(&it)) return -1;
    }

    return 0;
}

int item_pool_tests(void) {
    RUN_TEST(item_pool_sanity);
    RUN_TEST(item_pool_iter_sanity);
    RUN_TEST(item_pool_buf_sizing);
    return 0;
}

