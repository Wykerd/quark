#include <quark/net/tcp.h>
#include <quark/net/dns.h>
#include <stdlib.h>
#include <quark/no_malloc.h>

static void qrk__tcp_write_cb(uv_write_t *req, int status)
{
    qrk_tcp_t *tcp = req->handle->data;
    if (status != 0)
    {
        qrk_err_t err = {
            .code = status,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        goto exit;
    }

    if (tcp->on_write)
        tcp->on_write(tcp);

exit:
    qrk_free(tcp->m_ctx, req);
}

void qrk_tcp_write(qrk_tcp_t *tcp, qrk_rbuf_t *buf)
{
    uv_write_t *write = qrk_malloc(tcp->m_ctx, sizeof(uv_write_t));
    if (!write)
    {
        qrk_err_t err = {
            .code = QRK_E_OOM,
            .origin = QRK_EO_IMPL,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return;
    }
    int r = uv_write(write, (uv_stream_t*)&tcp->handle, (uv_buf_t *)buf, 1, qrk__tcp_write_cb);
    if (r != 0)
    {
        qrk_err_t err = {
            .code = r,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return;
    }
}

int qrk_tcp_init(qrk_tcp_t* tcp, qrk_loop_t* loop)
{
    memset(tcp, 0, sizeof(qrk_tcp_t));
    memcpy(tcp, loop, sizeof(qrk_loop_t));
    tcp->type = QRK_ET_TCP;

    int r = uv_tcp_init(tcp->loop, &tcp->handle);
    if (r != 0)
        return r;

    tcp->handle.data = tcp;
    tcp->write = qrk_tcp_write;

    return 0;
}

static void qrk__tcp_connect_cb(uv_connect_t *req, int status)
{
    qrk_tcp_t *tcp = req->handle->data;
    if (status != 0) {
        qrk_err_t err = {
            .code = status,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        goto exit;
    }

    if (tcp->on_connect)
        tcp->on_connect(tcp);
exit:
    qrk_free(tcp->m_ctx, req);
}

int qrk_tcp_connect(qrk_tcp_t* tcp, const struct sockaddr* addr)
{
    uv_connect_t *connect = qrk_malloc(tcp->m_ctx, sizeof(uv_connect_t));
    if (!connect)
    {
        qrk_err_t err = {
            .code = QRK_E_OOM,
            .origin = QRK_EO_IMPL,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return QRK_E_OOM;
    }

    int r = uv_tcp_connect(connect, &tcp->handle, addr, qrk__tcp_connect_cb);

    if (r != 0)
    {
        qrk_err_t err = {
            .code = r,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return r;
    }

    return 0;
}

typedef void (*qrk__tcp_resolve_cb)(qrk_tcp_t *sock, struct sockaddr* res);

static int qrk__tcp_resolve (qrk_tcp_t *tcp, const char *hostname, const char *port,
                             uv_getaddrinfo_cb resolve_cb, qrk__tcp_resolve_cb sync_resolve_cb)
{
    int n = qrk_dns_is_numeric_host_v(hostname);
    if (n)
    {
        int r;
        switch (n)
        {
            case 4:
            {
                struct sockaddr_in addr;
                r = uv_ip4_addr(hostname, port, &addr);
                if (!r)
                    sync_resolve_cb(tcp, (struct sockaddr *)&addr);
            }
            break;

            case 6:
            {
                struct sockaddr_in6 dst;
                r = uv_ip6_addr(hostname, atoi(port), &dst);
                if (!r)
                    sync_resolve_cb(tcp, (struct sockaddr *)&dst);
            }
            break;

            default:
            {
                qrk_err_t err = {
                    .code = QRK_E_UNREACHABLE,
                    .origin = QRK_EO_IMPL,
                    .target = tcp,
                    .target_type = tcp->type
                };
                qrk_err_emit(&err);
                return QRK_E_UNREACHABLE;
            }
        }

        if (unlikely(r != 0))
        {
            qrk_err_t err = {
                .code = r,
                .origin = QRK_EO_UV,
                .target = tcp,
                .target_type = tcp->type
            };
            qrk_err_emit(&err);
            return r;
        }

        return 0;
    }

    uv_getaddrinfo_t *req = qrk_malloc(tcp->m_ctx, sizeof(uv_getaddrinfo_t));
    if (!req)
    {
        qrk_err_t err = {
            .code = QRK_E_OOM,
            .origin = QRK_EO_IMPL,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return QRK_E_OOM;
    }
    req->data = tcp;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int r = uv_getaddrinfo(
        tcp->loop,
        req,
        resolve_cb,
        hostname, port,
        &hints
    );

    if (r != 0)
    {
        qrk_err_t err = {
            .code = r,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return r;
    }

    return 0;
}

static void qrk__tcp_getaddrinfo_cb (uv_getaddrinfo_t *req, int status, struct addrinfo *res)
{
    qrk_tcp_t *tcp = req->data;

    if (status != 0)
    {
        qrk_err_t err = {
            .code = status,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        goto exit;
    }

    qrk_tcp_connect(tcp, res->ai_addr);

exit:
    uv_freeaddrinfo(res);
    qrk_free(tcp->m_ctx, req);
}

int qrk_tcp_connect_host(qrk_tcp_t* tcp, const char* host, const char *port)
{
    return qrk__tcp_resolve(tcp, host, port, qrk__tcp_getaddrinfo_cb, qrk_tcp_connect);
}

static void qrk__tcp_alloc_cb (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    qrk_tcp_t *tcp = handle->data;
    buf->base = qrk_malloc(tcp->m_ctx, suggested_size);
    buf->len = suggested_size;
}

static void qrk__tcp_read_cb (uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
    qrk_tcp_t *tcp = handle->data;

    if (nread > 0)
    {
        if (tcp->on_read)
            tcp->on_read(tcp,(qrk_rbuf_t *)buf);
    }
    else
    {
        if (nread == UV_EOF)
        {
            qrk_tcp_shutdown(tcp);
            goto exit;
        }
        uv_read_stop(handle);
        qrk_err_t err = {
            .code = nread,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
    }
exit:
    qrk_free(tcp->m_ctx, buf->base);
}

int qrk_tcp_read_start(qrk_tcp_t* tcp)
{
    int r = uv_read_start((uv_stream_t *)&tcp->handle, qrk__tcp_alloc_cb, qrk__tcp_read_cb);
    if (r != 0)
    {
        qrk_err_t err = {
            .code = r,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return r;
    }

    return 0;
}

static void qrk__tcp_close_cb (uv_handle_t *handle)
{
    qrk_tcp_t *tcp = handle->data;
    if (tcp->on_close)
        tcp->on_close(tcp);
}

int qrk_tcp_shutdown(qrk_tcp_t* tcp)
{
    int r = uv_tcp_close_reset((uv_tcp_t *)&tcp->handle, qrk__tcp_close_cb);
    if (r != 0)
    {
        qrk_err_t err = {
            .code = r,
            .origin = QRK_EO_UV,
            .target = tcp,
            .target_type = tcp->type
        };
        qrk_err_emit(&err);
        return r;
    }
    return 0;
}
