#include <stdatomic.h>
#include <stdio.h>
#include "circular_buf.h"
#include "string_tape.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "memory.h"
#include "event_loop.h"
#include "item_pool.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#define MESSAGE_BUF_SIZE (200)
#define MAX_MESSAGE_LEN (1024 * 1024)
#define MESSAGE_CONTENT_SIZE ((MESSAGE_BUF_SIZE + 1) * MAX_MESSAGE_LEN)

#define MAX_CONNECTIONS (1024)

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

enum event_id {
    EVENT_ID_TCP_LISTEN,
    EVENT_ID_CONN_START,
    EVENT_ID_CONN_END = EVENT_ID_CONN_START + MAX_CONNECTIONS,
};

int set_socket_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -errno;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -errno;
    return 0;
}

int create_tcp_listener(int* fd, uint16_t *port) {
    int ret = 0;

    if (fd == NULL || port == NULL) return -EINVAL;

    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd < 0) goto ret_errno;

    struct sockaddr_in my_addr = {0};
    socklen_t len = sizeof(my_addr);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = 0;
    my_addr.sin_family = AF_INET;

    if (bind(*fd, (struct sockaddr*) &my_addr, len))
        goto ret_errno;

    if (listen(*fd, 1024))
        goto ret_errno;

    if ((ret = set_socket_nonblock(*fd)))
        goto err;

    if (getsockname(*fd, (struct sockaddr *)&my_addr, &len) < 0)
        goto ret_errno;

    *port = ntohs(my_addr.sin_port);

    return 0;

ret_errno:
    ret = -errno;
err:
    if (*fd > 0) close(*fd);
    *fd = -1;
    return ret;
}

struct connection_meta {
    int fd;
    size_t read_cursor;
};

struct connection_layout {
    struct connection_meta meta;
    uint8_t buffers_start[];
};

#define CONN_READ_BUF_SIZE (PAGE_SIZE - offsetof(struct connection_layout, buffers_start))
struct connection {
    struct connection_meta meta;
    uint8_t read_buf[CONN_READ_BUF_SIZE];
};

_Static_assert(sizeof(struct connection) % PAGE_SIZE == 0);

void connections_close(struct connection *connections,  struct item_pool *pool, size_t connection_id) {
    struct connection *connection = &connections[connection_id];
    close(connection->meta.fd);
    item_pool_free(pool, connection_id);

    reclaim_mem_range(connection, connection + 1);
}

struct chat_server {
    struct connection connections[MAX_CONNECTIONS] __attribute__((aligned(PAGE_SIZE)));

    struct history history;

    struct event_loop loop;

    int tcp_listener;
    uint16_t listener_port;

    DECLARE_ITEM_POOL_BUF(connection_pool_buf, MAX_CONNECTIONS);
    struct item_pool connection_pool;
};

int chat_server_listen(struct chat_server *server) {
    while (true) {
        int client_fd = accept(server->tcp_listener, NULL, NULL);

        if (client_fd < 0 && errno == EAGAIN) {
            return 0;
        } else if (client_fd < 0) {
            printf("Failed to accept, time to quit\n");
            return -errno;
        }

        printf("New client %d\n", client_fd);

        size_t connection_id = SIZE_MAX;
        if (item_pool_alloc(&server->connection_pool, &connection_id)) {
            goto drop_conn;
        }

        struct connection *connection = &server->connections[connection_id];
        connection->meta = (struct connection_meta) {
            .fd = client_fd,
            .read_cursor = 0,
        };

        // FIXME: Split function with shared error handling with item_pool_alloc
        if (set_socket_nonblock(client_fd)) {
            goto drop_conn;
        }

        if (event_loop_register(&server->loop, client_fd, EVENT_ID_CONN_START + connection_id)) goto drop_conn;
        continue;

drop_conn:
        printf("Dropping connection");

        if (connection_id != SIZE_MAX) {
            item_pool_free(&server->connection_pool, connection_id);
        }

        if (client_fd > 0)  {
            close(client_fd);
        }
    }
}

void chat_server_service_conn(struct chat_server *server, size_t connection_id) {
    struct connection *connection = &server->connections[connection_id];

    while (true) {
        ssize_t bytes_read = read(
                connection->meta.fd,
                connection->read_buf + connection->meta.read_cursor,
                CONN_READ_BUF_SIZE - connection->meta.read_cursor);

        printf("Bytes read %ld %d\n", bytes_read, errno);

        if (bytes_read < 0) {
            if (errno == EAGAIN) {
                return;
            }

            printf("Failed to read from connection (%d)\n", -errno);

            connections_close(server->connections, &server->connection_pool, connection_id);
            return;
        } else if (bytes_read == 0) {
            printf("Connection finished, dumping data\n");
            // FIXME: Check if read_cursor doesn't fit in an int printf is an angry boy
            printf("%.*s\n", (int)connection->meta.read_cursor, connection->read_buf);
            connections_close(server->connections, &server->connection_pool, connection_id);
            return;
        } else {
            connection->meta.read_cursor += (size_t)bytes_read;
        }
    }
}

int main(void) {
    assert(sysconf(_SC_PAGESIZE) == 1 << PAGE_SIZE_LOG2);

    int ret = 0;

    struct chat_server *server;
    if ((ret = posix_memalign((void**)&server, PAGE_SIZE, sizeof(struct chat_server)))) return ret;

    assert(alignof(server->connections) == PAGE_SIZE);
    assert((size_t)server->connections % PAGE_SIZE == 0);

    history_init_pinned(&server->history);
    server->loop = (struct event_loop) {0};
    server->tcp_listener = -1;
    server->listener_port = 0;

    // FIXME: Linear allocator API might be a little nicer
    server->connection_pool = item_pool_init(server->connection_pool_buf, MAX_CONNECTIONS);

    if ((ret = event_loop_init(&server->loop))) goto out;
    if ((ret = create_tcp_listener(&server->tcp_listener, &server->listener_port))) goto out;
    if ((ret = event_loop_register(&server->loop, server->tcp_listener, EVENT_ID_TCP_LISTEN))) goto out;

    printf("Listening on port %u\n", server->listener_port);

    while (true) {
        size_t event;

        if ((ret = event_loop_poll(&server->loop, &event))) goto out;

#define event_eq(id) (event == id)
#define event_between(a, b) (event >= a && event <= b)

        if (event_eq(EVENT_ID_TCP_LISTEN)) {
            if ((ret = chat_server_listen(server))) goto out;
        } else if (event_between(EVENT_ID_CONN_START, EVENT_ID_CONN_END)) {
            chat_server_service_conn(server, event - EVENT_ID_CONN_START);
        }

#undef event_eq
#undef event_between

    }

    //printf("first\n");
    //dump_status();

    //for (size_t i = 0; i < MESSAGE_CONTENT_SIZE / 25; i++) {
    //    if ((ret = history_push(history, "hello"))) return ret;
    //    if ((ret = history_push(history, "world"))) return ret;
    //    if ((ret = history_push(history, "we're"))) return ret;
    //    if ((ret = history_push(history, "so"))) return ret;
    //    if ((ret = history_push(history, "back"))) return ret;
    //}

    //printf("second\n");
    //dump_status();

    //history_reclaim_memory(history);
    //printf("third\n");
    //dump_status();

    //for (size_t i = 0; i < circular_buffer_len(&history->messages); i++) {
    //    char* message = circular_buffer_get(&history->messages, history->messages_buf, i);
    //    printf("%s\n", message);
    //}

out:
    event_loop_free(&server->loop);
    if (server->tcp_listener > 0) close(server->tcp_listener);
    if (ret != 0) {
        printf("Error: %d\n", ret);
    }

    // FIXME: close all active connections
    struct item_pool_iter conn_it = item_pool_iter(&server->connection_pool);
    while (item_pool_iter_next(&conn_it)) {
        struct connection *conn = &server->connections[conn_it.idx];
        close(conn->meta.fd);
    }

    free(server);

    return ret;
}
