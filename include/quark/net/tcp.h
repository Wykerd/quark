#ifndef QRK_NET_TCP_H
#define QRK_NET_TCP_H

#include <quark/std/stream.h>

typedef struct qrk_tcp_s qrk_tcp_t;

struct qrk_tcp_s {
    QRK_MEMORY_CONTEXT_FIELDS
    QRK_ERR_FIELDS
    QRK_LOOP_HANDLE_FIELD
    QRK_STREAM_FIELDS
    qrk_stream_cb on_close;
    uv_tcp_t handle;
    qrk_stream_cb on_connect;
    qrk_stream_cb on_connection;
};

void qrk_tcp_write(qrk_tcp_t *tcp, qrk_rbuf_t *buf);
int qrk_tcp_init(qrk_tcp_t* tcp, qrk_loop_t* loop);

int qrk_tcp_connect(qrk_tcp_t* tcp, const struct sockaddr* addr);
int qrk_tcp_connect_host(qrk_tcp_t* tcp, const char* host, const char *port);
// TODO: int qrk_tcp_connect_url(qrk_tcp_t* tcp, const char* url, size_t len);

int qrk_tcp_read_start(qrk_tcp_t* tcp);
int qrk_tcp_shutdown(qrk_tcp_t* tcp);

#endif //QRK_NET_TCP_H
