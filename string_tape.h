#pragma once

#include <stddef.h>

struct string_tape {
    char* data;
    size_t len;

    size_t start;
    size_t end;
};

static inline struct string_tape string_tape_init(char* buf, size_t len) {
    // Indexes 0 and 1 need to be valid. This could be fixed but a tape that
    // can store 2 chars doesn't seem very useful
    if (len < 2) return (struct string_tape) {0};

    return (struct string_tape) {
        .data = buf,
        .len = len,
        .start = 0,
        .end = 1,
    };
}

char* string_tape_push(struct string_tape *tape, char* data, size_t data_len);
int string_tape_mark_start(struct string_tape *tape, char* first);
int string_tape_reclaim_memory(struct string_tape *tape);
