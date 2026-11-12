#include "circular_buf_tests.c"
#include "string_tape_tests.c"
#include "item_pool_tests.c"
#include "hashmap_tests.c"

int any_test_failed = 0;

int main(void) {
    circular_buffer_tests();
    string_tape_tests();
    item_pool_tests();
    hashmap_tests();
    return any_test_failed;
}
