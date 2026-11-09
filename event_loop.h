#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>

#define EVENT_LOOP_MAX_EVENTS (1024)

struct event_loop {
    int fd;

    uint16_t event_idx;
    uint16_t buffered_events;

    struct epoll_event events[EVENT_LOOP_MAX_EVENTS];
};


int event_loop_init(struct event_loop *loop);
void event_loop_free(struct event_loop *loop);
int event_loop_poll(struct event_loop *loop, size_t *event);
int event_loop_register(struct event_loop *loop, int fd, size_t event_id);
