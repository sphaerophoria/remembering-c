#pragma once
#include <stdio.h>

extern int any_test_failed;

#define RUN_TEST(test_name) do { \
        printf("%s...", #test_name);\
        if (test_name()) { ;\
            printf("FAILED\n"); \
            any_test_failed = 1; \
        } else { \
            printf("SUCCESS\n"); \
        }; \
    } while(0);
