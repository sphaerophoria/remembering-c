#include "event_loop.h"
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

int event_loop_init(struct event_loop *loop) {
    int fd = epoll_create1(0);
    if (fd < 0) return -errno;

    loop->fd = fd;
    loop->event_idx = 0;
    loop->buffered_events = 0;

    return 0;
}

void event_loop_free(struct event_loop *loop) {
    if (loop->fd == 0) return;
    close(loop->fd);
}

int event_loop_poll(struct event_loop *loop, size_t *event) {
    while (true) {
        if (loop->event_idx < loop->buffered_events) {
            *event = loop->events[loop->event_idx++].data.u64;
            return 0;
        }

        int ret = epoll_wait(loop->fd, loop->events, EVENT_LOOP_MAX_EVENTS, -1);

        if (ret < 0) {
            if (errno == EINTR) continue;
            return -errno;
        }

        loop->event_idx = 0;
        loop->buffered_events = ret;
    }
}

int event_loop_register(struct event_loop *loop, int fd, size_t event_id) {
    struct epoll_event event = {
        .events = EPOLLIN | EPOLLOUT | EPOLLET,
        .data = {
            .u64 = event_id,
        }
    };

    if (epoll_ctl(loop->fd, EPOLL_CTL_ADD, fd, &event)) {
        return -errno;
    }

    return 0;
}
