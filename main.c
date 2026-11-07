#include <stdatomic.h>
#include <stdio.h>
#include "circular_buf.h"
#include "string_tape.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include "memory.h"

#include <string.h>
#include <errno.h>
#include <assert.h>

int test_fn(void) {
    printf("Hello world\n");
    return 0;
}
#define min(a,b) \
  ({ typeof(a) _a = (a); \
     typeof(b) _b = (b); \
    _a < _b ? _a : _b; })


#define MESSAGE_BUF_SIZE (200)
#define MAX_MESSAGE_LEN (1024 * 1024)
#define MESSAGE_CONTENT_SIZE ((MESSAGE_BUF_SIZE + 1) * MAX_MESSAGE_LEN)

struct history {
    char* messages_buf[MESSAGE_BUF_SIZE];
    struct circular_buffer messages;

    char message_content_buf[MESSAGE_CONTENT_SIZE];
    struct string_tape message_content;
};

void history_init_pinned(struct history* history) {
    history->messages = circular_buffer_init(MESSAGE_BUF_SIZE);
    history->message_content = string_tape_init(history->message_content_buf, MESSAGE_CONTENT_SIZE);
}

int history_push(struct history *history, char* data) {
    // Make sure previous dropped messages have their storage relcaimed
    int ret = 0;

    if (circular_buffer_len(&history->messages) > 0) {
        if ((ret = string_tape_mark_start(&history->message_content, circular_buffer_get(&history->messages, history->messages_buf, 0))))
            return ret;
    }

    char* stored = string_tape_push(&history->message_content, data, strlen(data));
    if (stored == NULL) return -ENOMEM;

    circular_buffer_push(&history->messages, history->messages_buf, stored);

    return 0;
}

int history_reclaim_memory(struct history *history) {
    return string_tape_reclaim_memory(&history->message_content);
}

int dump_status(void) {
    int ret = 0;
    char status_buf[16384];
    int status = open("/proc/self/status", O_RDONLY, 0);
    ssize_t read_bytes = read(status, status_buf, 16384);
    if (read_bytes < 0) {
        ret = -1;
        goto out;
    }

    printf("%*s\n", (int)read_bytes, status_buf);

out:
    close(status);
    return ret;
}

int main(void) {
    assert(sysconf(_SC_PAGESIZE) == 1 << PAGE_SIZE_LOG2);

    struct history *history = malloc(sizeof(struct history));
    history_init_pinned(history);

    int ret = 0;

    printf("first\n");
    dump_status();

    for (size_t i = 0; i < MESSAGE_CONTENT_SIZE / 25; i++) {
        if ((ret = history_push(history, "hello"))) return ret;
        if ((ret = history_push(history, "world"))) return ret;
        if ((ret = history_push(history, "we're"))) return ret;
        if ((ret = history_push(history, "so"))) return ret;
        if ((ret = history_push(history, "back"))) return ret;
    }

    printf("second\n");
    dump_status();

    history_reclaim_memory(history);
    printf("third\n");
    dump_status();

    for (size_t i = 0; i < circular_buffer_len(&history->messages); i++) {
        char* message = circular_buffer_get(&history->messages, history->messages_buf, i);
        printf("%s\n", message);
    }
}
