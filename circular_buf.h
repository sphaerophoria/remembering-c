#pragma once

#include <stddef.h>

// --- Public API ---

struct circular_buffer {
    size_t capacity;

    size_t push_head;
    size_t pop_head;
};

#define circular_buffer_init(_capacity) { \
    .capacity = (_capacity) \
}

#define circular_buffer_push(cb, buf, elem) do { \
        struct circular_buffer *_cb_push_cb = (cb); \
        buf[_cb_push_cb->push_head % _cb_push_cb->capacity] = (elem); \
        circular_buffer_inc_push_head(_cb_push_cb); \
    } while(0)

#define circular_buffer_get(cb, buf, idx) (buf)[circular_buffer_idx_to_buf_idx((cb), (idx))]

static inline size_t circular_buffer_len(struct circular_buffer const* cb) {
    return cb->push_head - cb->pop_head;
}

// --- Implementation details ---
static inline void circular_buffer_inc_push_head(struct circular_buffer* cb) {
    cb->push_head += 1;

    if (circular_buffer_len(cb) > cb->capacity) {
        cb->pop_head += 1;

        // This is probably unnecessary if you just always did wrapping logic, but whatever
        if (cb->pop_head >= cb->capacity) {
            cb->pop_head -= cb->capacity;
            cb->push_head -= cb->capacity;
        }
    }
}

static inline size_t circular_buffer_idx_to_buf_idx(struct circular_buffer const* cb, size_t idx) {
    return (idx + cb->pop_head) % cb->capacity;
}


