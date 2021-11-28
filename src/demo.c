#include <quark/std/alloc.h>
#include <quark/std/stream.h>
#include <quark/net/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void connected(qrk_tcp_t *tcp) {
    printf("connected\n");
    qrk_tcp_read_start(tcp);
    qrk_rbuf_t buf = {
            .base = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n",
            .len = sizeof("GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n") - 1
    };
    tcp->write(tcp, &buf);
}

void reading(qrk_tcp_t *tcp, qrk_rbuf_t *read) {
    fwrite(read->base, 1, read->len, stdout);
}

int main(int argc, char const *argv[])
{
    qrk_malloc_ctx_t mctx;

    qrk_malloc_ctx_new(&mctx);

    qrk_loop_t loop = qrk_loop_def(&mctx);

    qrk_tcp_t tcp;
    qrk_tcp_init(&tcp, &loop);
    tcp.on_connect = connected;
    tcp.on_read = reading;
    qrk_tcp_connect_host(&tcp, "www.example.com", "80");

    qrk_start(&loop);

    qrk_malloc_ctx_dump_leaks(&mctx);

    puts("DONE");
    return 0;
}
