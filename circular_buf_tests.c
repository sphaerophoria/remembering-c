#include "tests.h"
#include "circular_buf.h"
#include <string.h>

#define ROLLOVER_TEST_SIZE (4)
int cb_simple_rollover() {
    struct circular_buffer cb = circular_buffer_init(ROLLOVER_TEST_SIZE);
    int cb_buf[ROLLOVER_TEST_SIZE];

    if (circular_buffer_len(&cb) != 0) return -1;

    circular_buffer_push(&cb, cb_buf, 1);
    if (circular_buffer_len(&cb) != 1) return -1;

    circular_buffer_push(&cb, cb_buf, 2);
    if (circular_buffer_len(&cb) != 2) return -1;

    circular_buffer_push(&cb, cb_buf, 3);
    if (circular_buffer_len(&cb) != 3) return -1;

    circular_buffer_push(&cb, cb_buf, 4);
    if (circular_buffer_len(&cb) != 4) return -1;

    circular_buffer_push(&cb, cb_buf, 5);
    if (circular_buffer_len(&cb) != 4) return -1;

    if (circular_buffer_get(&cb, cb_buf, 0) != 2) return -1;
    if (circular_buffer_get(&cb, cb_buf, 1) != 3) return -1;
    if (circular_buffer_get(&cb, cb_buf, 2) != 4) return -1;
    if (circular_buffer_get(&cb, cb_buf, 3) != 5) return -1;

    return 0;
}

struct cb_with_struct_struct {
    int something;
    char* something_else;
};
int cb_with_struct() {
    struct circular_buffer cb = circular_buffer_init(ROLLOVER_TEST_SIZE);
    struct cb_with_struct_struct cb_buf[ROLLOVER_TEST_SIZE];

    circular_buffer_push(&cb, cb_buf, ((struct cb_with_struct_struct) {
            .something = 4,
            .something_else = "hello",
    }));

    if (circular_buffer_len(&cb) != 1) return -1;
    struct cb_with_struct_struct out = circular_buffer_get(&cb, cb_buf, 0);

    if (out.something != 4) return -1;
    if (strcmp(out.something_else, "hello") != 0) return -1;

    return 0;
}

void circular_buffer_tests(void) {
    RUN_TEST(cb_simple_rollover);
    RUN_TEST(cb_with_struct);
}
