#include <stdatomic.h>
#include <stdio.h>
#include <errno.h>
#include "circular_buf.h"

int test_fn(void) {
    printf("Hello world\n");
    return 0;
}

#define MESSAGE_BUF_SIZE (4)

int main(void) {
    char* message_buf[MESSAGE_BUF_SIZE];
    struct circular_buffer messages = circular_buffer_init(MESSAGE_BUF_SIZE);

    circular_buffer_push(&messages, message_buf, "hello");
    circular_buffer_push(&messages, message_buf, "world");
    circular_buffer_push(&messages, message_buf, "we're");
    circular_buffer_push(&messages, message_buf, "so");
    circular_buffer_push(&messages, message_buf, "back");

    for (size_t i = 0; i < circular_buffer_len(&messages); i++) {
        char* message = circular_buffer_get(&messages, message_buf, i);
        printf("%s\n", message);
    }
}
