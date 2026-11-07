#include "string_tape.h"
#include "tests.h"
#include <errno.h>

static int string_tape_zero_init(void) {
    struct string_tape tape = {0};

    if (string_tape_push(&tape, "hello", 5) != NULL) return -1;
    if (string_tape_push(&tape, "", 0) != NULL) return -1;
    return 0;
}

static int string_tape_sanity(void) {
    char buf[1024];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));

    size_t initial_tape_end = tape.end;
    if (string_tape_push(&tape, "hello", 5) != tape.data + 1) return -1;
    if (tape.end != initial_tape_end + 6) return -1;

    return 0;
}

static int string_tape_start_before_end(void) {
    char buf[1024];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));

    tape.start = 55;
    tape.end = 999;

    if (string_tape_push(&tape, "hello", 5) != tape.data + 999) return -1;
    if (tape.end != 1005) return -1;

    return 0;
}

static int string_tape_end_hits_len(void) {
    char buf[1024];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));

    tape.start = 999;
    tape.end = 1016;

    if (string_tape_push(&tape, "hello", 5) != tape.data + 1016) return -1;
    if (tape.end != 1022) return -1;

    if (string_tape_push(&tape, "hello", 5) != tape.data) return -1;
    if (tape.end != 6) return -1;

    return 0;
}

static int string_tape_end_string_too_long(void) {
    char buf[16];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));
    tape.start = 1;
    tape.end = 5;

    if (string_tape_push(&tape, "the quick brown fox jumped over the lazy dog", 44) != NULL) return -1;
    if (tape.end != 5) return -1;
    if (tape.start != 1) return -1;

    return 0;
}

static int string_tape_end_hits_len_then_start(void) {
    char buf[1024];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));
    tape.start = 1;
    tape.end = 1000;

    if (string_tape_push(&tape, "the quick brown fox jumped over the lazy dog", 44) != NULL) return -1;
    if (tape.end != 1000) return -1;
    if (tape.start != 1) return -1;

    return 0;
}

static int string_tape_end_hits_start_then_len(void) {
    char buf[1024];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));
    tape.start = 1003;
    tape.end = 1000;

    if (string_tape_push(&tape, "the quick brown fox jumped over the lazy dog", 44) != NULL) return -1;
    if (tape.end != 1000) return -1;
    if (tape.start != 1003) return -1;

    return 0;
}


static int string_tape_end_lines_up_with_start(void) {
    // Since we only track end/start we can't tell the difference between
    // end == start == full and end == start == empty.
    //
    // We just say that end == start never happens and stop thinking about it
    char buf[1024];
    struct string_tape tape = string_tape_init(buf, sizeof(buf));
    tape.start = 1005;
    tape.end = 1000;

    if (string_tape_push(&tape, "hello", 5) != NULL) return -1;
    if (tape.end != 1000) return -1;
    if (tape.start != 1005) return -1;

    return 0;
}


int string_tape_tests(void) {
    RUN_TEST(string_tape_zero_init);
    RUN_TEST(string_tape_sanity);
    RUN_TEST(string_tape_start_before_end);
    RUN_TEST(string_tape_end_hits_len);
    RUN_TEST(string_tape_end_string_too_long);
    RUN_TEST(string_tape_end_hits_len_then_start);
    RUN_TEST(string_tape_end_hits_start_then_len);
    RUN_TEST(string_tape_end_lines_up_with_start);
    return 0;
}
