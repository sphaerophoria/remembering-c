#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>

#define PAGE_SIZE_LOG2 (12)

static inline void* round_down_by_pow_2(void* ptr, uint8_t pow) {
    size_t ptr_int = (size_t)ptr;
    size_t mask = (1 << pow) - 1;
    ptr_int &= ~mask;
    return (void*) ptr_int;
}

static inline void* round_up_by_pow_2(void* ptr, uint8_t pow) {
    // (x + page_size - 1) & ~(page_size - 1) for align up
    size_t ptr_int = (size_t)ptr;
    size_t mask = (1 << pow) - 1;
    ptr_int += mask;
    ptr_int &= ~mask;
    return (void*)ptr_int;
}

static inline int reclaim_mem_range(void* start, void* end) {
    // FIXME: Calc page size
    start = round_up_by_pow_2(start, PAGE_SIZE_LOG2);
    end = round_down_by_pow_2(end, PAGE_SIZE_LOG2);

    if (end <= start) return 0;

    return madvise(start, end - start, MADV_DONTNEED);
}
