#include "circular_buf_tests.c"

int any_test_failed = 0;

int main(void) {
    circular_buffer_tests();
    return any_test_failed;
}
