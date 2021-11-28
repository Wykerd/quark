#include <quark/net/tls.h>

static
int qrk__tls_init_intr(qrk_tls_t *tls, qrk_stream_t *stream) {
    tls->type = QRK_ET_TLS;
    tls->stream = stream;
    tls->write = qrk_tls_write;
    tls->stream->child = tls;
    tls->stream->on_read = qrk_tls_read_underlying;

    if (!qrk_str_malloc(&tls->enc_buf, tls->m_ctx, QRK_SSL_IO_BUF_SIZE))
        return QRK_E_OOM;

    tls->ssl = SSL_new(tls->sec_ctx.ssl_ctx);

    tls->rbio = qrk_crypto_bio_new(tls->m_ctx);
    tls->wbio = qrk_crypto_bio_new(tls->m_ctx);
    SSL_set_bio(tls->ssl, tls->rbio, tls->wbio);
    return 0;
}

int qrk_tls_init (qrk_tls_t* tls, qrk_stream_t* stream)
{
    memset(tls, 0, sizeof(qrk_tls_t));
    memcpy(tls, stream, sizeof(qrk_loop_t));
    unsigned long r;
    r = qrk_sec_ctx_init(&tls->sec_ctx, stream->m_ctx, TLS_method(), 0, 0);
    if (r)
        return r;
    r = qrk_sec_ctx_set_cipher_suites(&tls->sec_ctx, QRK_SSL_DEFAULT_CIPHER_SUITES);
    if (r)
        return r;
    r = qrk_sec_ctx_set_ciphers(&tls->sec_ctx, QRK_SSL_DEFAULT_CIPHER_LISTS);
    if (r)
        return r;
    r = qrk_sec_ctx_add_root_certs(&tls->sec_ctx);
    if (r)
        return r;

    return qrk__tls_init_intr(tls, stream);
}

int qrk_tls_init2 (qrk_tls_t* tls, qrk_stream_t* stream, qrk_secure_ctx_t *sec_ctx)
{
    memset(tls, 0, sizeof(qrk_tls_t));
    memcpy(tls, stream, sizeof(qrk_loop_t));
    tls->sec_ctx = *sec_ctx;
    return qrk__tls_init_intr(tls, stream);
}

static int qrk__tls_flush_wbio (qrk_tls_t *tls)
{
    char buf[QRK_SSL_IO_BUF_SIZE];
    int r = 0;
    ERR_clear_error();
    do
    {
        r = BIO_read(tls->wbio, buf, sizeof(buf));
        if (r > 0)
        {
            qrk_str_t bf = { buf, r };
            tls->stream->write(tls->stream, &bf);
        }
        else if (!BIO_should_retry(tls->wbio))
        {
            qrk_err_t err = {
                .code = ERR_get_error(),
                .origin = QRK_EO_OPENSSL,
                .target = tls,
                .target_type = tls->type
            };
            qrk_err_emit(&err);
            return err.code;
        }
    } while (r > 0);

    return 0;
}

static void qrk__tls_flush_enc_buf(qrk_tls_t *tls)
{
    int status;
    ERR_clear_error();
    while (tls->enc_buf.len > 0)
    {
        int r = SSL_write(tls->ssl, tls->enc_buf.base, tls->enc_buf.len);
        status = SSL_get_error(tls->ssl, r);

        if (r > 0)
        {
            // consume bytes
            qrk_str_shift(&tls->enc_buf, r);
            r = qrk__tls_flush_wbio(tls);
            if (r)
                return;
        }

        if (!((status == 0) || (status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ)))
        {
            qrk_err_t err = {
                .code = status,
                .origin = QRK_EO_OPENSSL,
                .target = tls,
                .target_type = tls->type
            };
            qrk_err_emit(&err);
            return;
        }

        if (r == 0)
            return;
    }
}

void qrk_tls_write (qrk_tls_t* tls, qrk_rbuf_t* buf)
{
    qrk_str_push_back(&tls->enc_buf, buf->base, buf->len);

    if (!SSL_is_init_finished(tls->ssl))
        return;

    return qrk__tls_flush_enc_buf(tls);
}

int qrk__tls_handshake (qrk_tls_t *tls)
{
    int r = 0,
        status = 0;

    ERR_clear_error();
    if (!SSL_is_init_finished(tls->ssl))
    {
        r = SSL_do_handshake(tls->ssl);
        status = SSL_get_error(tls->ssl, r);
        if ((status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ))
        {
            r = qrk__tls_flush_wbio(tls);
            if (r)
                return r;
        }
    }
    if (!((status == 0) || (status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ)))
    {
        qrk_err_t err = {
            .code = status,
            .origin = QRK_EO_OPENSSL,
            .target_type = tls->type,
            .target = tls
        };
        qrk_err_emit(&err);
        return err.code;
    }
    return 0;
}

void qrk_tls_read_underlying (qrk_stream_t *stream, qrk_rbuf_t *rbuf)
{
    qrk_tls_t *tls = stream->child;
    char buf[QRK_SSL_IO_BUF_SIZE];
    int r, status;
    size_t len = rbuf->len;
    char *src = rbuf->base;
    ERR_clear_error();
    while (len > 0)
    {
        r = BIO_write(tls->rbio, src, len);
        if (r <= 0)
        {
            qrk_err_t err = {
                .code = ERR_get_error(),
                .origin = QRK_EO_OPENSSL,
                .target = tls,
                .target_type = tls->type
            };
            qrk_err_emit(&err);
            return;
        };
        src += r;
        len -= r;
        if (!SSL_is_init_finished(tls->ssl))
        {
            int rr = qrk__tls_handshake(tls);
            if (rr)
                return;
            if (!SSL_is_init_finished(tls->ssl))
                return;
            else
                qrk__tls_flush_enc_buf(tls);
        }

        do
        {
            r = SSL_read(tls->ssl, buf, sizeof(buf));
            if (r > 0)
            {
                if (tls->on_read)
                {
                    qrk_rbuf_t obuf = {
                        buf, r
                    };
                    tls->on_read(tls, &obuf);
                }
            }
            else if (r < 0)
                break;
        } while (r > 0);

        if ((SSL_get_shutdown(tls->ssl) & SSL_RECEIVED_SHUTDOWN) ||
            (SSL_get_shutdown(tls->ssl) & SSL_SENT_SHUTDOWN))
        {
            /* Shut down */
            if (tls->on_close)
                tls->on_close(tls);

            return;
        }

        status = SSL_get_error(tls->ssl, r);
        if ((status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ))
        {
            r = qrk__tls_flush_wbio(tls);
            if (r)
                return;
        }

        if (!((status == 0) || (status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ)))
        {
            qrk_err_t err = {
                .code = status,
                .origin = QRK_EO_OPENSSL,
                .target = tls,
                .target_type = tls->type
            };
            qrk_err_emit(&err);
            return;
        };
    }
}

int qrk_tls_connect(qrk_tls_t *tls)
{
    SSL_set_connect_state(tls->ssl);
    return qrk__tls_handshake(tls);
}

int qrk_tls_accept(qrk_tls_t *tls)
{
    SSL_set_accept_state(tls->ssl);
    return 0;
}

int qrk_tls_connect_with_sni (qrk_tls_t *tls, const char *host)
{
    SSL_set_connect_state(tls->ssl);
    int r = SSL_set_tlsext_host_name(tls->ssl, host);
    if (!r)
    {
        qrk_err_t err = {
            .code = ERR_get_error(),
            .origin = QRK_EO_OPENSSL,
            .target = tls,
            .target_type = tls->type
        };
        qrk_err_emit(&err);
        return err.code;
    };
    return qrk__tls_handshake(tls);
}

void qrk_tls_free(qrk_tls_t *tls)
{
    SSL_free(tls->ssl); /* This method also frees the BIOs */
    qrk_str_free(&tls->enc_buf);
}

int qrk_tls_shutdown (qrk_tls_t *tls)
{
    ERR_clear_error();
    int r = SSL_shutdown(tls->ssl);
    int status = SSL_get_error(tls->ssl, r);
    if ((status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ))
    {
        r = qrk__tls_flush_wbio(tls);
        if (r)
            return r;
    }

    return !((status == 0) || (status == SSL_ERROR_WANT_WRITE) || (status == SSL_ERROR_WANT_READ));
}
