#include <errno.h>

#include "hashmap.h"
#include "tests.h"
#include "string.h"

struct sanity_kv {
    uint64_t key;
    int32_t value;
};

// FIXME: Actually hash something lol
#define sanity_hash_fn(x) ((uint64_t)x)
#define sanity_eql_fn(_key, value) ((uint64_t)_key == (value)->key)
#define sanity_get(map, items, key) hashmap_get_impl(map, items, key, sanity_hash_fn, sanity_eql_fn)


static int hashmap_sanity(void) {
    DECLARE_HASHMAP_BUFFERS(hashmap_buf, 16);
    struct hashmap map = hashmap_init(hashmap_buf, 16);
    struct sanity_kv items[16] = {0};

    {
        struct hashmap_ref ref = sanity_get(&map, items, 5);
        if (ref.idx != SIZE_MAX) return -1;

        if (hashmap_insert(&ref)) return -1;
        items[ref.idx] = (struct sanity_kv) {
            .key = 5,
            .value = 7,
        };
    }

    {
        struct hashmap_ref ref = sanity_get(&map, items, 5);
        if (ref.idx == SIZE_MAX) return -1;
        if (items[ref.idx].key != 5) return -1;
        if (items[ref.idx].value != 7) return -1;
        hashmap_remove(&ref);
    }

    {
        struct hashmap_ref ref = sanity_get(&map, items, 5);
        if (ref.idx != SIZE_MAX) return -1;
    }

    return 0;
}

static int hashmap_insertion_ref_matches_get_ref(void) {
    DECLARE_HASHMAP_BUFFERS(hashmap_buf, 16);
    struct hashmap map = hashmap_init(hashmap_buf, 16);
    struct sanity_kv items[16] = {0};

    struct hashmap_ref after_insert = sanity_get(&map, items, 5);
    if (after_insert.idx != SIZE_MAX) return -1;
    if (hashmap_insert(&after_insert)) return -1;
    items[after_insert.idx] = (struct sanity_kv) {
        .key = 5,
        .value = 7,
    };

    struct hashmap_ref after_get = sanity_get(&map, items, 5);

    if (after_get.idx != after_insert.idx) return -1;
    if (after_get.hash != after_insert.hash) return -1;
    if (after_get.node != after_insert.node) return -1;
    if (after_get.prev != after_insert.prev) return -1;
    if (after_get.map != after_insert.map) return -1;

    return 0;
}

static int hashmap_remove_invalid(void) {
    DECLARE_HASHMAP_BUFFERS(hashmap_buf, 16);
    struct hashmap map = hashmap_init(hashmap_buf, 16);
    struct sanity_kv items[16] = {0};

    struct hashmap_ref ref = sanity_get(&map, items, 5);
    hashmap_remove(&ref);
    return 0;
}

static int hashmap_at_capacity(void) {
    DECLARE_HASHMAP_BUFFERS(hashmap_buf, 16);
    struct hashmap map = hashmap_init(hashmap_buf, 16);
    struct sanity_kv items[16] = {0};

    for (size_t i = 0; i < 16; i++) {
        struct hashmap_ref ref = sanity_get(&map, items, i);
        if (ref.idx != SIZE_MAX) return -1;
        if (hashmap_insert(&ref)) return -1;
        items[ref.idx] = (struct sanity_kv) {
            .key = (uint64_t)i,
            .value = (int)i,
        };
    }

    struct hashmap_ref ref = sanity_get(&map, items, 99);
    if (ref.idx != SIZE_MAX) return -1;
    if (hashmap_insert(&ref) == 0) return -1;
    return 0;
}

static int hashmap_bucket_collision(void) {
    DECLARE_HASHMAP_BUFFERS(hashmap_buf, 16);
    struct hashmap map = hashmap_init(hashmap_buf, 16);
    struct sanity_kv items[16] = {0};

    // Keys 5 and 21 land in the same bucket (both % 16 == 5).
    struct hashmap_ref ref5 = sanity_get(&map, items, 5);
    if (hashmap_insert(&ref5)) return -1;
    items[ref5.idx] = (struct sanity_kv) { .key = 5, .value = 100 };

    struct hashmap_ref ref21 = sanity_get(&map, items, 21);
    if (ref21.idx != SIZE_MAX) return -1;
    if (hashmap_insert(&ref21)) return -1;
    items[ref21.idx] = (struct sanity_kv) { .key = 21, .value = 200 };

    // Both keys must still be reachable after the colliding insert.
    struct hashmap_ref check5 = sanity_get(&map, items, 5);
    if (check5.idx != ref5.idx) return -1;
    struct hashmap_ref check21 = sanity_get(&map, items, 21);
    if (check21.idx != ref21.idx) return -1;

    // Removing the bucket head must not orphan its sibling.
    hashmap_remove(&check5);
    struct hashmap_ref check5_again = sanity_get(&map, items, 5);
    if (check5_again.idx != SIZE_MAX) return -1;
    struct hashmap_ref check21_again = sanity_get(&map, items, 21);
    if (check21_again.idx != ref21.idx) return -1;

    return 0;
}

static int hashmap_insert_existing_rejected(void) {
    DECLARE_HASHMAP_BUFFERS(hashmap_buf, 16);
    struct hashmap map = hashmap_init(hashmap_buf, 16);
    struct sanity_kv items[16] = {0};

    struct hashmap_ref ref = sanity_get(&map, items, 5);
    if (hashmap_insert(&ref)) return -1;
    items[ref.idx] = (struct sanity_kv) { .key = 5, .value = 100 };

    struct hashmap_ref found = sanity_get(&map, items, 5);
    if (found.idx != ref.idx) return -1;
    if (hashmap_insert(&found) != -EEXIST) return -1;

    return 0;
}

static void hashmap_tests(void) {
    RUN_TEST(hashmap_sanity);
    RUN_TEST(hashmap_insertion_ref_matches_get_ref);
    RUN_TEST(hashmap_remove_invalid);
    RUN_TEST(hashmap_at_capacity);
    RUN_TEST(hashmap_bucket_collision);
    RUN_TEST(hashmap_insert_existing_rejected);
}
