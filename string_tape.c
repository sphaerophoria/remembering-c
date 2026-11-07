#include "string_tape.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "memory.h"

static bool string_passes_nowrap(size_t start_idx, size_t strlen, size_t compare_idx) {
    return start_idx <= compare_idx && start_idx + strlen >= compare_idx;
}

char* string_tape_push(struct string_tape *tape, char* data, size_t data_len) {
    size_t string_start = tape->end;
    size_t strlen = data_len + 1; // Don't forget \0

    if (string_passes_nowrap(string_start, strlen, tape->start)) {
        return NULL;
    }

    if (string_start + strlen >= tape->len) {
        string_start = 0;
    }

    if (string_passes_nowrap(string_start, strlen, tape->start)) {
        return NULL;
    }

    char* tape_string = tape->data + string_start;
    memcpy(tape_string, data, data_len);
    *(tape_string + data_len) = 0;

    tape->end = string_start + strlen;

    return tape_string;
}

// Tell me where the oldest string I need to remember is
int string_tape_mark_start(struct string_tape *tape, char* first) {
    if (first < tape->data || first >= tape->data + tape->len) return -EINVAL;
    tape->start = first - tape->data;
    return 0;
}

int string_tape_reclaim_memory(struct string_tape *tape) {
    int ret = 0;
    if (tape->start > tape->end) {
        if ((ret = reclaim_mem_range(
            tape->data + tape->end,
            tape->data + tape->start
        ))) return ret;
    } else {
        if ((ret = reclaim_mem_range(
            tape->data + tape->end,
            tape->data + tape->len
        ))) return ret;

        if ((ret = reclaim_mem_range(
            tape->data,
            tape->data + tape->start
        ))) return ret;
    }

    return 0;
}
