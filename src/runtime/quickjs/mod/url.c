#include <quark/runtime/quickjs/vm.h>
#include <quark/runtime/quickjs/mod/url.h>
#include <quark/std/utils.h>
#include <quark/url/url.h>
#include <quickjs.h>
#include <string.h>
#include <stdlib.h>

static JSClassID qrk_qjs_url_class_id;
static JSClassID qrk_qjs_urlsearchparams_class_id;
static JSClassID qrk_qjs_urlsearchparams_iterator_class_id;

typedef struct qrk_qjs_url_ctx_s {
    qrk_url_parser_t parser;
    qrk_url_t *url;
    JSValue search_params;
    JSContext *ctx;
} qrk_qjs_url_ctx_t;

typedef struct qrk_qjs_urlsearchparams_ctx_s {
    qrk_buf_t list;
    qrk_qjs_url_ctx_t *parent;
} qrk_qjs_urlsearchparams_ctx_t;

typedef struct qrk_qjs_search_iter_ctx_t {
    qrk_qjs_urlsearchparams_ctx_t *uctx;
    size_t i;
    JSIteratorKindEnum kind;
} qrk_qjs_search_iter_ctx_t;

static void qrk_qjs_url_finalizer(JSRuntime *rt, JSValue val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(val, qrk_qjs_url_class_id);
    qrk_qjs_rt_t *qrt = JS_GetRuntimeOpaque(rt);
    if (likely(url_ctx)) {
        JS_FreeValue(url_ctx->ctx, url_ctx->search_params);
        qrk_url_free(url_ctx->url);
        qrk_free(qrt->mctx, url_ctx->url);
        qrk_free(qrt->mctx, url_ctx);
    }
}

static void qrk_qjs_urlsearchparams_finalizer (JSRuntime *rt, JSValue val)
{
    qrk_qjs_urlsearchparams_ctx_t *params = JS_GetOpaque(val, qrk_qjs_urlsearchparams_class_id);
    qrk_qjs_rt_t *qrt = JS_GetRuntimeOpaque(rt);
    if (likely(params)) {
        for (int i = 0; i < params->list.nmemb; i++)
        {
            qrk_kv_t * kv = (qrk_kv_t *)qrk_buf_get(&params->list, i);
            qrk_str_free(&kv->key);
            qrk_str_free(&kv->value);
        }
        qrk_buf_free(&params->list);
        qrk_free(qrt->mctx, params);
    }
}

static void qrk_qjs_search_iterator_finalizer (JSRuntime *rt, JSValue val)
{
    qrk_qjs_search_iter_ctx_t *url_ctx = JS_GetOpaque(val, qrk_qjs_urlsearchparams_iterator_class_id);
    qrk_qjs_rt_t *qrt = JS_GetRuntimeOpaque(rt);
    if (likely(url_ctx)) {
        qrk_free(qrt->mctx, url_ctx);
    }
}

static void qrk_qjs_url_mark (JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(val, qrk_qjs_url_class_id);
    if (likely(url_ctx)) {
        JS_MarkValue(rt, url_ctx->search_params, mark_func);
    }
}

static JSClassDef qrk_qjs_url_class = {
    "URL",
    .finalizer = qrk_qjs_url_finalizer,
    .gc_mark = qrk_qjs_url_mark,
};

static JSClassDef qrk_qjs_urlsearchparams_class = {
    "URLSearchParams",
    .finalizer = qrk_qjs_urlsearchparams_finalizer
};

static JSClassDef qrk_qjs_urlsearchparams_iterator_class = {
    "URLSearchParams Iterator",
    .finalizer = qrk_qjs_search_iterator_finalizer
};

static JSValue qrk_qjs_url_stringifier (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    qrk_qjs_rt_t *qrt = JS_GetContextOpaque(ctx);

    qrk_str_t url_str;
    if (!qrk_str_malloc(&url_str, qrt->mctx, 128))
        return JS_EXCEPTION;

    if (qrk_url_serialize(url_ctx->url, &url_str, 0)) {
        qrk_str_free(&url_str);
        return JS_EXCEPTION;
    }

    JSValue ret = JS_NewStringLen(ctx, url_str.base, url_str.len);

    qrk_str_free(&url_str);

    return ret;
}

static JSValue qrk_qjs_url_stringifier_func (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return qrk_qjs_url_stringifier(ctx, this_val);
}

static JSValue qrk_qjs_url_set_href (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *url_str = JS_ToCString(ctx, value);
    if (unlikely(!url_str))
        return JS_EXCEPTION;

    qrk_url_t *url = NULL;

    qrk_rbuf_t url_str_buf = {
        .base = (char *) url_str,
        .len = strlen(url_str)
    };
    if (qrk_url_parse_basic(&url_ctx->parser, &url_str_buf, NULL, &url, 0) != 0) {
       if (url)
           qrk_url_free(url);
        JS_FreeCString(ctx, url_str);
        return JS_ThrowTypeError(ctx, "Failed to set the 'href' property on URL: Invalid URL");
    }
    JS_FreeCString(ctx, url_str);

    qrk_url_free(url_ctx->url);
    qrk_free(url_ctx->parser.m_ctx, url_ctx->url);

    url_ctx->url = url;

    qrk_qjs_urlsearchparams_ctx_t *params = JS_GetOpaque(url_ctx->search_params, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!params))
        return JS_EXCEPTION;

    qrk_buf_t list_buf;

    if (qrk_url_form_urlencoded_parse((qrk_rbuf_t *) &url->query, &list_buf, params->list.m_ctx))
        return JS_ThrowOutOfMemory(ctx);

    for (int i = 0; i < params->list.nmemb; i++)
    {
        qrk_kv_t * kv = (qrk_kv_t *)qrk_buf_get(&params->list, i);
        qrk_str_free(&kv->key);
        qrk_str_free(&kv->value);
    }
    qrk_buf_free(&params->list);

    params->list.nmemb = list_buf.nmemb;
    params->list.base = list_buf.base;
    params->list.m_ctx = list_buf.m_ctx;
    params->list.size = list_buf.size;
    params->list.memb_size = list_buf.memb_size;

    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_origin (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_rt_t *qrt = JS_GetContextOpaque(ctx);
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    qrk_str_t str;
    if (!qrk_str_malloc(&str, qrt->mctx, url_ctx->url->host.str.len + 128))
        return JS_ThrowOutOfMemory(ctx);

    qrk_html_origin_tuple_t *origin = NULL;

    if (qrk_url_origin(url_ctx->url, &origin, qrt->mctx))
    {
fail_oom:
        if (origin)
        {
            qrk_html_origin_tuple_free(origin);
            qrk_free(qrt->mctx, origin);
        }
        qrk_str_free(&str);
        return JS_ThrowOutOfMemory(ctx);
    }

    if (qrk_html_origin_serialize(&str, origin))
        goto fail_oom;

    JSValue ret = JS_NewStringLen(ctx, str.base, str.len);

    qrk_str_free(&str);
    qrk_html_origin_tuple_free(origin);
    qrk_free(qrt->mctx, origin);
    return ret;
}

static JSValue qrk_qjs_url_get_protocol (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if ((url_ctx->url->scheme.size <= url_ctx->url->scheme.len) ||
        (url_ctx->url->scheme.base[url_ctx->url->scheme.len] != ':'))
    {
        if (!qrk_str_putc(&url_ctx->url->query, ':'))
            return JS_ThrowOutOfMemory(ctx);

        url_ctx->url->query.len--;
    }

    return JS_NewStringLen(ctx, url_ctx->url->scheme.base, url_ctx->url->scheme.len + 1);
}

static JSValue qrk_qjs_url_set_protocol (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *protocol = JS_ToCString(ctx, value);
    if (unlikely(!protocol))
        return JS_EXCEPTION;

    size_t protocol_len = strlen(protocol);

    char *protocol_end = qrk_malloc(url_ctx->parser.m_ctx, protocol_len + 1);
    if (!protocol_end)
    {
        JS_FreeCString(ctx, protocol);
        return JS_ThrowOutOfMemory(ctx);
    }

    strcpy(protocol_end, protocol);
    protocol_end[protocol_len] = ':';

    qrk_rbuf_t protocol_str = {
        .base = (char *) protocol_end,
        .len = protocol_len + 1
    };

    qrk_url_parse_basic(&url_ctx->parser, &protocol_str, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_SCHEME_START);

    qrk_free(url_ctx->parser.m_ctx, protocol_end);

    JS_FreeCString(ctx, protocol);

    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_username (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    return JS_NewStringLen(ctx, url_ctx->url->username.base, url_ctx->url->username.len);
}

static JSValue qrk_qjs_url_set_username (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *username = JS_ToCString(ctx, value);
    if (unlikely(!username))
        return JS_EXCEPTION;

    if ((url_ctx->url->host.type == QRK_URL_HOST_NULL) ||
        (url_ctx->url->host.type == QRK_URL_HOST_EMPTY) ||
        (QRK_URL_HOST_IS_DOMAIN_OR_OPAQUE(&url_ctx->url->host) && url_ctx->url->host.str.len == 0) ||
        (url_ctx->url->flags & QRK_URL_FLAG_IS_FILE))
        return JS_DupValue(ctx, value);

    url_ctx->url->username.len = 0;

    if (!qrk_str_puts(&url_ctx->url->username, username))
    {
        JS_FreeCString(ctx, username);
        return JS_ThrowOutOfMemory(ctx);
    }

    if (url_ctx->url->username.len)
        url_ctx->url->flags |= QRK_URL_FLAG_HAS_USERNAME;
    else
        url_ctx->url->flags &= ~QRK_URL_FLAG_HAS_USERNAME;

    JS_FreeCString(ctx, username);
    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_password (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    return JS_NewStringLen(ctx, url_ctx->url->password.base, url_ctx->url->password.len);
}

static JSValue qrk_qjs_url_set_password (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *password = JS_ToCString(ctx, value);
    if (unlikely(!password))
        return JS_EXCEPTION;

    if ((url_ctx->url->host.type == QRK_URL_HOST_NULL) ||
        (url_ctx->url->host.type == QRK_URL_HOST_EMPTY) ||
        (QRK_URL_HOST_IS_DOMAIN_OR_OPAQUE(&url_ctx->url->host) && url_ctx->url->host.str.len == 0) ||
        (url_ctx->url->flags & QRK_URL_FLAG_IS_FILE))
        return JS_DupValue(ctx, value);

    url_ctx->url->password.len = 0;

    if (!qrk_str_puts(&url_ctx->url->password, password))
    {
        JS_FreeCString(ctx, password);
        return JS_ThrowOutOfMemory(ctx);
    }

    if (url_ctx->url->password.len)
        url_ctx->url->flags |= QRK_URL_FLAG_HAS_PASSWORD;
    else
        url_ctx->url->flags &= ~QRK_URL_FLAG_HAS_PASSWORD;

    JS_FreeCString(ctx, password);
    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_host (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->host.type == QRK_URL_HOST_NULL || url_ctx->url->host.type == QRK_URL_HOST_EMPTY)
        return JS_NewStringLen(ctx, "", 0);

    qrk_str_t host;

    if (!qrk_str_malloc(&host, url_ctx->parser.m_ctx, url_ctx->url->host.str.len + 6))
        return JS_ThrowOutOfMemory(ctx);

    if (qrk_url_host_serialize(&host, &url_ctx->url->host))
    {
        qrk_str_free(&host);
        return JS_ThrowOutOfMemory(ctx);
    }

    if (url_ctx->url->port != -1)
    {
        if (qrk_str_printf(&host, ":%d", url_ctx->url->port))
        {
            qrk_str_free(&host);
            return JS_ThrowOutOfMemory(ctx);
        }
    }

    JSValue host_str = JS_NewStringLen(ctx, host.base, host.len);

    qrk_str_free(&host);

    return host_str;
}

static JSValue qrk_qjs_url_set_host (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
        return JS_DupValue(ctx, value);

    const char *host = JS_ToCString(ctx, value);
    if (unlikely(!host))
        return JS_EXCEPTION;

    qrk_rbuf_t host_rbuf = {
        .base = (char *)host,
        .len = strlen(host)
    };

    qrk_url_parse_basic(&url_ctx->parser, &host_rbuf, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_HOST);

    JS_FreeCString(ctx, host);
    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_hostname (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->host.type == QRK_URL_HOST_NULL || url_ctx->url->host.type == QRK_URL_HOST_EMPTY)
        return JS_NewStringLen(ctx, "", 0);

    qrk_str_t host;

    if (!qrk_str_malloc(&host, url_ctx->parser.m_ctx, url_ctx->url->host.str.len + 6))
        return JS_ThrowOutOfMemory(ctx);

    if (qrk_url_host_serialize(&host, &url_ctx->url->host))
    {
        qrk_str_free(&host);
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue host_str = JS_NewStringLen(ctx, host.base, host.len);

    qrk_str_free(&host);

    return host_str;
}

static JSValue qrk_qjs_url_set_hostname (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
        return JS_DupValue(ctx, value);

    const char *host = JS_ToCString(ctx, value);
    if (unlikely(!host))
        return JS_EXCEPTION;

    qrk_rbuf_t host_rbuf = {
            .base = (char *)host,
            .len = strlen(host)
    };

    qrk_url_parse_basic(&url_ctx->parser, &host_rbuf, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_HOSTNAME);

    JS_FreeCString(ctx, host);
    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_port (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->port < 0)
        return JS_NewStringLen(ctx, "", 0);

    qrk_str_t port;

    if (!qrk_str_malloc(&port, url_ctx->parser.m_ctx, url_ctx->url->host.str.len + 6))
        return JS_ThrowOutOfMemory(ctx);

    if (!qrk_str_printf(&port, "%d", url_ctx->url->port))
    {
        qrk_str_free(&port);
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue host_str = JS_NewStringLen(ctx, port.base, port.len);

    qrk_str_free(&port);

    return host_str;
}

static JSValue qrk_qjs_url_set_port (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if ((url_ctx->url->host.type == QRK_URL_HOST_NULL) ||
        (url_ctx->url->host.type == QRK_URL_HOST_EMPTY) ||
        (QRK_URL_HOST_IS_DOMAIN_OR_OPAQUE(&url_ctx->url->host) && url_ctx->url->host.str.len == 0) ||
        (url_ctx->url->flags & QRK_URL_FLAG_IS_FILE))
        return JS_DupValue(ctx, value);

    const char *port = JS_ToCString(ctx, value);
    if (unlikely(!port))
        return JS_EXCEPTION;

    qrk_rbuf_t port_rbuf = {
        .base = (char *)port,
        .len = strlen(port)
    };

    if (port_rbuf.len == 0)
    {
        url_ctx->url->port = -1;
        return JS_DupValue(ctx, value);
    }

    qrk_url_parse_basic(&url_ctx->parser, &port_rbuf, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_PORT);

    JS_FreeCString(ctx, port);
    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_pathname (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    qrk_str_t path;

    if (!qrk_str_malloc(&path, url_ctx->parser.m_ctx, 128))
        return JS_ThrowOutOfMemory(ctx);

    if (qrk_url_serialize_path(url_ctx->url, &path))
    {
        qrk_str_free(&path);
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue pathname = JS_NewStringLen(ctx, path.base, path.len);

    qrk_str_free(&path);

    return pathname;
}

static JSValue qrk_qjs_url_set_pathname (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *path = JS_ToCString(ctx, value);
    if (unlikely(!path))
        return JS_EXCEPTION;

    if (url_ctx->url->flags & QRK_URL_FLAG_PATH_IS_OPAQUE)
        return JS_DupValue(ctx, value);

    if (url_ctx->url->path.nmemb != 0)
        for (size_t i = 0; i < url_ctx->url->path.nmemb; i++)
            qrk_str_free((qrk_str_t *)(url_ctx->url->path.base + (i * url_ctx->url->path.memb_size)));

    url_ctx->url->path.nmemb = 0;

    qrk_rbuf_t path_rbuf = {
        .base = (char *)path,
        .len = strlen(path)
    };

    qrk_url_parse_basic(&url_ctx->parser, &path_rbuf, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_PATH_START);

    JS_FreeCString(ctx, path);

    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_search (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->query.len == 0)
        return JS_NewStringLen(ctx, "", 0);

    qrk_str_t search;
    if (!qrk_str_malloc(&search, url_ctx->parser.m_ctx, url_ctx->url->query.len + 1))
        return JS_ThrowOutOfMemory(ctx);

    if (!qrk_str_putc(&search, '?'))
    {
        qrk_str_free(&search);
        return JS_ThrowOutOfMemory(ctx);
    }

    if (!qrk_str_push_back(&search, url_ctx->url->query.base, url_ctx->url->query.len))
    {
        qrk_str_free(&search);
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue search_str = JS_NewStringLen(ctx, search.base, search.len);

    qrk_str_free(&search);

    return search_str;
}

static JSValue qrk_qjs_url_set_search (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *search = JS_ToCString(ctx, value);
    if (unlikely(!search))
        return JS_EXCEPTION;

    qrk_rbuf_t search_rbuf = {
        .base = (char *)search,
        .len = strlen(search)
    };

    if (search_rbuf.len != 0)
    {
        if (search_rbuf.base[0] == '?')
        {
            search_rbuf.base++;
            search_rbuf.len--;
        }
    }

    qrk_qjs_urlsearchparams_ctx_t *params = JS_GetOpaque(url_ctx->search_params, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!params))
        return JS_EXCEPTION;

    for (int i = 0; i < params->list.nmemb; i++)
    {
        qrk_kv_t * kv = (qrk_kv_t *)qrk_buf_get(&params->list, i);
        qrk_str_free(&kv->key);
        qrk_str_free(&kv->value);
    }
    qrk_buf_free(&params->list);

    params->list.nmemb = 0;

    url_ctx->url->query.len = 0;

    if (search_rbuf.len == 0)
    {
        JS_FreeCString(ctx, search);
        return JS_DupValue(ctx, value);
    }

    qrk_buf_t list_buf;

    if (qrk_url_form_urlencoded_parse(&search_rbuf, &list_buf, params->list.m_ctx))
        return JS_ThrowOutOfMemory(ctx);

    params->list.nmemb = list_buf.nmemb;
    params->list.base = list_buf.base;
    params->list.m_ctx = list_buf.m_ctx;
    params->list.size = list_buf.size;
    params->list.memb_size = list_buf.memb_size;

    qrk_url_parse_basic(&url_ctx->parser, &search_rbuf, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_QUERY);

    JS_FreeCString(ctx, search);

    return JS_DupValue(ctx, value);
}

static JSValue qrk_qjs_url_get_searchparams (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    return JS_DupValue(ctx, url_ctx->search_params);
}

static JSValue qrk_qjs_url_get_hash (JSContext *ctx, JSValueConst this_val)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    if (url_ctx->url->fragment.len == 0)
        return JS_NewStringLen(ctx, "", 0);

    qrk_str_t hash;
    if (!qrk_str_malloc(&hash, url_ctx->parser.m_ctx, url_ctx->url->fragment.len + 1))
        return JS_ThrowOutOfMemory(ctx);

    if (!qrk_str_putc(&hash, '#'))
    {
        qrk_str_free(&hash);
        return JS_ThrowOutOfMemory(ctx);
    }

    if (!qrk_str_push_back(&hash, url_ctx->url->fragment.base, url_ctx->url->fragment.len))
    {
        qrk_str_free(&hash);
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue fragment_str = JS_NewStringLen(ctx, hash.base, hash.len);

    qrk_str_free(&hash);

    return fragment_str;
}

static JSValue qrk_qjs_url_set_hash (JSContext *ctx, JSValueConst this_val, JSValueConst value)
{
    qrk_qjs_url_ctx_t *url_ctx = JS_GetOpaque(this_val, qrk_qjs_url_class_id);
    if (unlikely(!url_ctx))
        return JS_EXCEPTION;

    const char *hash = JS_ToCString(ctx, value);
    if (unlikely(!hash))
        return JS_EXCEPTION;

    qrk_rbuf_t hash_rbuf = {
        .base = (char *)hash,
        .len = strlen(hash)
    };

    url_ctx->url->fragment.len = 0;

    if (hash_rbuf.len != 0)
    {
        if (hash_rbuf.base[0] == '#')
            hash_rbuf.base++;
    }
    else
    {
        JS_FreeCString(ctx, hash);
        return JS_DupValue(ctx, value);
    }

    qrk_url_parse_basic(&url_ctx->parser, &hash_rbuf, NULL, &url_ctx->url, QRK_URL_PARSER_STATE_FRAGMENT);

    JS_FreeCString(ctx, hash);

    return JS_DupValue(ctx, value);
}

static const JSCFunctionListEntry qrk_qjs_url_proto_funcs[] = {
    JS_CGETSET_DEF("href", qrk_qjs_url_stringifier, qrk_qjs_url_set_href),
    JS_CGETSET_DEF("origin", qrk_qjs_url_get_origin, NULL),
    JS_CGETSET_DEF("protocol", qrk_qjs_url_get_protocol, qrk_qjs_url_set_protocol),
    JS_CGETSET_DEF("username", qrk_qjs_url_get_username, qrk_qjs_url_set_username),
    JS_CGETSET_DEF("password", qrk_qjs_url_get_password, qrk_qjs_url_set_password),
    JS_CGETSET_DEF("host", qrk_qjs_url_get_host, qrk_qjs_url_set_host),
    JS_CGETSET_DEF("hostname", qrk_qjs_url_get_hostname, qrk_qjs_url_set_hostname),
    JS_CGETSET_DEF("port", qrk_qjs_url_get_port, qrk_qjs_url_set_port),
    JS_CGETSET_DEF("pathname", qrk_qjs_url_get_pathname, qrk_qjs_url_set_pathname),
    JS_CGETSET_DEF("search", qrk_qjs_url_get_search, qrk_qjs_url_set_search),
    JS_CGETSET_DEF("searchParams", qrk_qjs_url_get_searchparams, NULL),
    JS_CGETSET_DEF("hash", qrk_qjs_url_get_hash, qrk_qjs_url_set_hash),
    JS_CFUNC_DEF("toString", 0, qrk_qjs_url_stringifier_func),
    JS_CFUNC_DEF("toJSON", 0, qrk_qjs_url_stringifier_func)
};

static void qrk_qjs_search_update (JSContext *ctx, qrk_qjs_urlsearchparams_ctx_t *uctx)
{
    if (uctx->parent == NULL)
        return;

    uctx->parent->url->query.len = 0;

    if (!qrk_url_form_urlencoded_serialize(&uctx->parent->url->query, &uctx->list))
        JS_ThrowOutOfMemory(ctx);
};

static JSValue qrk_qjs_search_append (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    if (argc < 2)
        return JS_ThrowTypeError(ctx, "Failed to execute 'append' on 'URLSearchParams': 2 arguments required, but only 0 present.");

    const char *key = JS_ToCString(ctx, argv[0]);
    if (unlikely(key == NULL))
        return JS_EXCEPTION;
    const char *val = JS_ToCString(ctx, argv[1]);
    if (unlikely(val == NULL))
    {
        JS_FreeCString(ctx, key);
        return JS_EXCEPTION;
    }

    size_t key_len = strlen(key);
    size_t val_len = strlen(val);

    qrk_kv_t kv;
    if (!qrk_str_malloc(&kv.key, uctx->list.m_ctx, key_len))
        goto oom;
    if (!qrk_str_malloc(&kv.value, uctx->list.m_ctx, val_len))
    {
        qrk_str_free(&kv.key);
        goto oom;
    }

    memcpy(kv.key.base, key, key_len);
    memcpy(kv.value.base, val, val_len);
    kv.key.len = key_len;
    kv.value.len = val_len;

    if (!qrk_buf_push_back1(&uctx->list, &kv))
    {
        qrk_str_free(&kv.key);
        qrk_str_free(&kv.value);
        goto oom;
    }

    qrk_qjs_search_update(ctx, uctx);
    return JS_UNDEFINED;
oom:
    JS_FreeCString(ctx, key);
    JS_FreeCString(ctx, val);
    return JS_ThrowOutOfMemory(ctx);
}

static JSValue qrk_qjs_search_delete (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    if (argc < 1)
        return JS_ThrowTypeError(ctx, "Failed to execute 'delete' on 'URLSearchParams': 1 argument required, but only 0 present.");

    const char *key = JS_ToCString(ctx, argv[0]);
    if (unlikely(key == NULL))
        return JS_EXCEPTION;

    size_t key_len = strlen(key);

    for (size_t i = 0; i < uctx->list.nmemb; i++)
    {
        qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
        if (kv->key.len == key_len && !memcmp(kv->key.base, key, key_len))
        {
            qrk_str_free(&kv->key);
            qrk_str_free(&kv->value);
            qrk_buf_remove(&uctx->list, i);
            i--;
        }
    }

    JS_FreeCString(ctx, key);
    qrk_qjs_search_update(ctx, uctx);

    return JS_UNDEFINED;
}

static JSValue qrk_qjs_search_get (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    if (argc < 1)
        return JS_ThrowTypeError(ctx, "Failed to execute 'get' on 'URLSearchParams': 1 argument required, but only 0 present.");

    const char *key = JS_ToCString(ctx, argv[0]);
    if (unlikely(key == NULL))
        return JS_EXCEPTION;

    size_t key_len = strlen(key);

    for (size_t i = 0; i < uctx->list.nmemb; i++)
    {
        qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
        if (kv->key.len == key_len && !memcmp(kv->key.base, key, key_len))
        {
            JS_FreeCString(ctx, key);
            return JS_NewStringLen(ctx, kv->value.base, kv->value.len);
        }
    }

    JS_FreeCString(ctx, key);

    return JS_UNDEFINED;
}

static JSValue qrk_qjs_search_get_all (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    if (argc < 1)
        return JS_ThrowTypeError(ctx, "Failed to execute 'getAll' on 'URLSearchParams': 1 argument required, but only 0 present.");

    const char *key = JS_ToCString(ctx, argv[0]);
    if (unlikely(key == NULL))
        return JS_EXCEPTION;

    size_t key_len = strlen(key);

    JSValue arr = JS_NewArray(ctx);

    int64_t idx = 0;

    for (size_t i = 0; i < uctx->list.nmemb; i++)
    {
        qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
        if (kv->key.len == key_len && !memcmp(kv->key.base, key, key_len))
        {
            JS_SetPropertyInt64(ctx, arr, idx++, JS_NewStringLen(ctx, kv->value.base, kv->value.len));
        }
    }

    JS_FreeCString(ctx, key);

    return arr;
}

static JSValue qrk_qjs_search_has (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    if (argc < 1)
        return JS_ThrowTypeError(ctx, "Failed to execute 'has' on 'URLSearchParams': 1 argument required, but only 0 present.");

    const char *key = JS_ToCString(ctx, argv[0]);
    if (unlikely(key == NULL))
        return JS_EXCEPTION;

    size_t key_len = strlen(key);

    for (size_t i = 0; i < uctx->list.nmemb; i++)
    {
        qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
        if (kv->key.len == key_len && !memcmp(kv->key.base, key, key_len))
        {
            JS_FreeCString(ctx, key);
            return JS_TRUE;
        }
    }

    JS_FreeCString(ctx, key);

    return JS_FALSE;
}

static JSValue qrk_qjs_search_set (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    if (argc < 2)
        return JS_ThrowTypeError(ctx, "Failed to execute 'set' on 'URLSearchParams': 2 arguments required, but only 0 present.");

    const char *key = JS_ToCString(ctx, argv[0]);
    if (unlikely(key == NULL))
        return JS_EXCEPTION;
    const char *val = JS_ToCString(ctx, argv[1]);
    if (unlikely(val == NULL))
    {
        JS_FreeCString(ctx, key);
        return JS_EXCEPTION;
    }

    size_t key_len = strlen(key);
    size_t val_len = strlen(val);

    size_t idx = -1;

    for (size_t i = 0; i < uctx->list.nmemb; i++)
    {
        qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
        if (kv->key.len == key_len && !memcmp(kv->key.base, key, key_len))
        {
            kv->value.len = 0;
            if (!qrk_str_puts(&kv->value, val))
                goto oom;
            idx = i;
            break;
        }
    }

    if (idx != -1)
        for (size_t i = idx + 1; i < uctx->list.nmemb; i++)
        {
            qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
            if (kv->key.len == key_len && !memcmp(kv->key.base, key, key_len))
            {
                qrk_str_free(&kv->key);
                qrk_str_free(&kv->value);
                qrk_buf_remove(&uctx->list, i);
                i--;
            }
        }
    else
    {
        qrk_kv_t kv;
        if (!qrk_str_malloc(&kv.key, uctx->list.m_ctx, key_len))
            goto oom;
        if (!qrk_str_malloc(&kv.value, uctx->list.m_ctx, val_len))
        {
            qrk_str_free(&kv.key);
            goto oom;
        }

        memcpy(kv.key.base, key, key_len);
        memcpy(kv.value.base, val, val_len);
        kv.key.len = key_len;
        kv.value.len = val_len;

        if (!qrk_buf_push_back1(&uctx->list, &kv))
        {
            qrk_str_free(&kv.key);
            qrk_str_free(&kv.value);
            goto oom;
        }
    }

    JS_FreeCString(ctx, key);
    JS_FreeCString(ctx, val);
    qrk_qjs_search_update(ctx, uctx);

    return JS_UNDEFINED;
oom:
    JS_FreeCString(ctx, key);
    JS_FreeCString(ctx, val);
    return JS_ThrowOutOfMemory(ctx);
}

// TODO: move to utils
static int qrK_qjs_str_cmp (const void *a, const void *b)
{
    const qrk_kv_t *kv_a = a;
    const qrk_kv_t *kv_b = b;

    size_t min_len = kv_a->key.len < kv_b->key.len ? kv_a->key.len : kv_b->key.len;

    int cmp = memcmp(kv_a->key.base, kv_b->key.base, min_len);

    if (cmp == 0)
    {
        if (kv_a->key.len < kv_b->key.len)
            return -1;
        else if (kv_a->key.len > kv_b->key.len)
            return 1;
        else
            return 0;
    }

    return cmp;
}

static JSValue qrk_qjs_search_sort (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    // TODO: locale sorting with icu ucol
    qsort(uctx->list.base, uctx->list.nmemb, uctx->list.memb_size, qrK_qjs_str_cmp);

    qrk_qjs_search_update(ctx, uctx);
    return JS_UNDEFINED;
}

static JSValue qrk_qjs_search_stringifier (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    size_t initial_len = uctx->parent != NULL ? uctx->parent->url->query.len : uctx->list.nmemb * 64;

    qrk_str_t str;

    if (!qrk_str_malloc(&str, uctx->list.m_ctx, initial_len))
        return JS_ThrowOutOfMemory(ctx);

    if (!qrk_url_form_urlencoded_serialize(&str, &uctx->list))
    {
        qrk_str_free(&str);
        return JS_ThrowOutOfMemory(ctx);
    }

    JSValue ret = JS_NewStringLen(ctx, str.base, str.len);
    qrk_str_free(&str);

    return ret;
}

static int check_function(JSContext *ctx, JSValueConst obj)
{
    if (likely(JS_IsFunction(ctx, obj)))
        return 0;
    JS_ThrowTypeError(ctx, "not a function");
    return -1;
}

static JSValue qrk_qjs_search_for_each (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    JSValueConst func, this_arg;
    JSValue ret, args[3];

    func = argv[0];
    if (argc > 1)
        this_arg = argv[1];
    else
        this_arg = JS_UNDEFINED;
    if (check_function(ctx, func))
        return JS_EXCEPTION;

    for (size_t i = 0; i < uctx->list.nmemb; i++)
    {
        qrk_kv_t *kv = uctx->list.base + (i * uctx->list.memb_size);
        /* must duplicate in case the record is deleted */
        args[0] = JS_NewStringLen(ctx, kv->value.base, kv->value.len);
        args[1] = JS_NewStringLen(ctx, kv->key.base, kv->key.len);
        args[2] = (JSValue)this_val;
        ret = JS_Call(ctx, func, this_arg, 3, (JSValueConst *)args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        if (JS_IsException(ret))
            return ret;
        JS_FreeValue(ctx, ret);
    }
    return JS_UNDEFINED;
}

static JSValue qrk_qjs_create_search_iterator(JSContext *ctx, JSValueConst this_val,
                                              int argc, JSValueConst *argv, int magic)
{
    qrk_qjs_urlsearchparams_ctx_t *uctx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_class_id);
    if (unlikely(!uctx))
        return JS_EXCEPTION;

    qrk_qjs_rt_t *qrt = JS_GetContextOpaque(ctx);

    qrk_qjs_search_iter_ctx_t *ictx = qrk_malloc(qrt->mctx, sizeof(qrk_qjs_search_iter_ctx_t));
    if (ictx == NULL)
        return JS_ThrowOutOfMemory(ctx);

    JSValue obj = JS_NewObjectClass(ctx, (int)qrk_qjs_urlsearchparams_iterator_class_id);
    if (JS_IsException(obj))
    {
        qrk_free(qrt->mctx, ictx);
        return obj;
    }

    ictx->uctx = uctx;
    ictx->i = 0;
    ictx->kind = magic;

    JS_SetOpaque(obj, ictx);

    return obj;
}

static JSCFunctionListEntry qrk_qjs_urlsearchparams_proto_funcs[] = {
    JS_CFUNC_DEF("append", 2, qrk_qjs_search_append),
    JS_CFUNC_DEF("delete", 1, qrk_qjs_search_delete),
    JS_CFUNC_DEF("get", 1, qrk_qjs_search_get),
    JS_CFUNC_DEF("getAll", 1, qrk_qjs_search_get_all),
    JS_CFUNC_DEF("has", 1, qrk_qjs_search_has),
    JS_CFUNC_DEF("set", 2, qrk_qjs_search_set),
    JS_CFUNC_DEF("sort", 0, qrk_qjs_search_sort),
    // stringifier
    JS_CFUNC_DEF("toString", 0, qrk_qjs_search_stringifier),
    // iterable<USVString, USVString>
    JS_CFUNC_DEF("forEach", 1, qrk_qjs_search_for_each),
    JS_CFUNC_MAGIC_DEF("values", 0, qrk_qjs_create_search_iterator, JS_ITERATOR_KIND_VALUE ),
    JS_CFUNC_MAGIC_DEF("[Symbol.iterator]", 0, qrk_qjs_create_search_iterator, JS_ITERATOR_KIND_KEY_AND_VALUE ),
    JS_CFUNC_MAGIC_DEF("keys", 0, qrk_qjs_create_search_iterator, JS_ITERATOR_KIND_KEY ),
    JS_CFUNC_MAGIC_DEF("entries", 0, qrk_qjs_create_search_iterator, JS_ITERATOR_KIND_KEY_AND_VALUE ),
};

static JSValue qrk_qjs_search_next(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv,
                                  int *pdone, int magic)
{
    qrk_qjs_search_iter_ctx_t *ictx = JS_GetOpaque(this_val, qrk_qjs_urlsearchparams_iterator_class_id);
    if (unlikely(!ictx))
        return JS_EXCEPTION;

    if (ictx->i >= ictx->uctx->list.nmemb)
        goto done;

    qrk_kv_t *kv = ictx->uctx->list.base + (ictx->i * ictx->uctx->list.memb_size);
    JSValue ret;
    switch (ictx->kind) {
        case JS_ITERATOR_KIND_VALUE:
            ret = JS_NewStringLen(ctx, kv->value.base, kv->value.len);
            break;
        case JS_ITERATOR_KIND_KEY:
            ret = JS_NewStringLen(ctx, kv->key.base, kv->key.len);
            break;
        case JS_ITERATOR_KIND_KEY_AND_VALUE:
            ret = JS_NewArray(ctx);
            if (JS_IsException(ret))
                return ret;
            JS_SetPropertyUint32(ctx, ret, 0, JS_NewStringLen(ctx, kv->key.base, kv->key.len));
            JS_SetPropertyUint32(ctx, ret, 1, JS_NewStringLen(ctx, kv->value.base, kv->value.len));
            break;
    }
    ictx->i++;
    *pdone = 0;
    return ret;
done:
    *pdone = 1;
    return JS_UNDEFINED;
}

static JSCFunctionListEntry qrk_qjs_urlsearchparams_iterator_proto_funcs[] = {
    JS_ITERATOR_NEXT_DEF("next", 0, qrk_qjs_search_next, 0),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "URLSearchParams Iterator", JS_PROP_CONFIGURABLE)
};

static JSValue qrk_qjs_urlseachparams_init_url (JSContext *ctx, qrk_rbuf_t *query, qrk_qjs_url_ctx_t *parent)
{
    qrk_qjs_rt_t *qrt = JS_GetContextOpaque(ctx);

    JSValue obj = JS_NewObjectClass(ctx, (int)qrk_qjs_urlsearchparams_class_id);
    if (JS_IsException(obj))
        return obj;

    qrk_qjs_urlsearchparams_ctx_t *params = qrk_malloc(qrt->mctx, sizeof(qrk_qjs_urlsearchparams_ctx_t));
    if (!params)
        return JS_ThrowOutOfMemory(ctx);

    if (qrk_url_form_urlencoded_parse(query, &params->list, qrt->mctx))
        return JS_ThrowOutOfMemory(ctx);

    params->parent = parent;

    JS_SetOpaque(obj, params);

    return obj;
}

static JSValue qrk_qjs_urlsearchparams_ctor (JSContext *ctx, JSValueConst new_target,
                                 int argc, JSValueConst *argv)
{
    JSValue ret;
    qrk_qjs_rt_t *qrt = JS_GetContextOpaque(ctx);
    qrk_qjs_urlsearchparams_ctx_t *params = qrk_malloc(qrt->mctx, sizeof(qrk_qjs_urlsearchparams_ctx_t));
    if (!params)
        return JS_ThrowOutOfMemory(ctx);

    params->list.base = NULL;
    params->parent = NULL;

    if (argc >= 1)
    {
        JSValue init = argv[0];
        if (JS_IsArray(ctx, init))
        {
            if (!qrk_buf_malloc(&params->list, qrt->mctx, sizeof(qrk_kv_t), 1)) {
                qrk_free(qrt->mctx, params);
                return JS_ThrowOutOfMemory(ctx);
            }

            JSValue js_len = JS_GetPropertyStr(ctx, init, "length");
            uint64_t len;
            if (JS_ToIndex(ctx, &len, js_len))
            {
                JS_FreeValue(ctx, js_len);
                ret = JS_EXCEPTION;
                goto fail;
            }
            JS_FreeValue(ctx, js_len);

            if (!qrk_buf_resize(&params->list, len))
            {
                ret = JS_EXCEPTION;
                goto fail;
            }

            for (uint32_t i = 0; i < len; i++)
            {
                JSValue js_item = JS_GetPropertyUint32(ctx, init, i);
                if (JS_IsException(js_item))
                {
                    ret = JS_EXCEPTION;
                    goto fail;
                }

                if (!JS_IsArray(ctx, js_item))
                {
                    JS_FreeValue(ctx, js_item);
                    ret = JS_ThrowTypeError(ctx, "Failed to construct 'URLSearchParams': The provided value is not an sequence.");
                    goto fail;
                }

                JSValue sub_len_js = JS_GetPropertyStr(ctx, js_item, "length");
                uint64_t sub_len;
                if (JS_ToIndex(ctx, &sub_len, sub_len_js))
                {
                    JS_FreeValue(ctx, sub_len_js);
                    JS_FreeValue(ctx, js_item);
                    ret = JS_EXCEPTION;
                    goto fail;
                }
                JS_FreeValue(ctx, sub_len_js);

                if (sub_len != 2)
                {
                    JS_FreeValue(ctx, js_item);
                    ret = JS_ThrowTypeError(ctx, "Failed to construct 'URLSearchParams': Sequence initializer must only contain pair elements.");
                    goto fail;
                }

                const char *key = JS_ToCString(ctx, JS_GetPropertyUint32(ctx, js_item, 0));
                if (!key)
                {
                    JS_FreeValue(ctx, js_item);
                    ret = JS_EXCEPTION;
                    goto fail;
                }
                const char *value = JS_ToCString(ctx, JS_GetPropertyUint32(ctx, js_item, 1));
                if (!value)
                {
                    JS_FreeValue(ctx, js_item);
                    JS_FreeCString(ctx, key);
                    ret = JS_EXCEPTION;
                    goto fail;
                }

                qrk_kv_t kv;
                size_t keylen = strlen(key),
                       valuelen = strlen(value);
                if (!qrk_str_malloc(&kv.key, qrt->mctx, keylen))
                {
                    JS_FreeValue(ctx, js_item);
                    JS_FreeCString(ctx, key);
                    JS_FreeCString(ctx, value);
                    ret = JS_ThrowOutOfMemory(ctx);
                    goto fail;
                }
                if (!qrk_str_malloc(&kv.value, qrt->mctx, valuelen))
                {
                    JS_FreeValue(ctx, js_item);
                    JS_FreeCString(ctx, key);
                    JS_FreeCString(ctx, value);
                    qrk_str_free(&kv.key);
                    ret = JS_ThrowOutOfMemory(ctx);
                    goto fail;
                }
                memcpy(kv.key.base, key, keylen);
                memcpy(kv.value.base, value, valuelen);
                kv.key.len = keylen;
                kv.value.len = valuelen;

                if (!qrk_buf_push_back1(&params->list, &kv))
                {
                    JS_FreeValue(ctx, js_item);
                    JS_FreeCString(ctx, key);
                    JS_FreeCString(ctx, value);
                    qrk_str_free(&kv.key);
                    qrk_str_free(&kv.value);
                    ret = JS_ThrowOutOfMemory(ctx);
                    goto fail;
                }

                JS_FreeCString(ctx, key);
                JS_FreeCString(ctx, value);
                JS_FreeValue(ctx, js_item);
            }
        }
        else if (JS_IsObject(init))
        {
            if (!qrk_buf_malloc(&params->list, qrt->mctx, sizeof(qrk_kv_t), 1)) {
                qrk_free(qrt->mctx, params);
                return JS_ThrowOutOfMemory(ctx);
            }

            JSPropertyEnum *ptab;
            uint32_t plen;
            if (JS_GetOwnPropertyNames(ctx, &ptab, &plen, init, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY))
            {
                ret = JS_EXCEPTION;
                goto fail;
            }

            for (uint32_t i = 0; i < plen; i++)
            {
                JSValue prop = JS_GetProperty(ctx, init, ptab[i].atom);
                if (JS_IsException(prop))
                {
                    JS_FreeValue(ctx, prop);
                    ret = JS_EXCEPTION;
                    goto fail;
                }

                const char *key = JS_AtomToCString(ctx, ptab[i].atom);
                if (!key)
                {
                    JS_FreeValue(ctx, prop);
                    ret = JS_EXCEPTION;
                    goto fail;
                }
                const char *value = JS_ToCString(ctx, prop);
                if (!value)
                {
                    JS_FreeValue(ctx, prop);
                    JS_FreeCString(ctx, key);
                    ret = JS_EXCEPTION;
                    goto fail;
                }

                qrk_kv_t kv;
                size_t keylen = strlen(key),
                        valuelen = strlen(value);
                if (!qrk_str_malloc(&kv.key, qrt->mctx, keylen))
                {
                    JS_FreeValue(ctx, prop);
                    JS_FreeCString(ctx, key);
                    JS_FreeCString(ctx, value);
                    ret = JS_ThrowOutOfMemory(ctx);
                    goto fail;
                }
                if (!qrk_str_malloc(&kv.value, qrt->mctx, valuelen))
                {
                    JS_FreeValue(ctx, prop);
                    JS_FreeCString(ctx, key);
                    JS_FreeCString(ctx, value);
                    qrk_str_free(&kv.key);
                    ret = JS_ThrowOutOfMemory(ctx);
                    goto fail;
                }
                memcpy(kv.key.base, key, keylen);
                memcpy(kv.value.base, value, valuelen);
                kv.key.len = keylen;
                kv.value.len = valuelen;

                if (!qrk_buf_push_back1(&params->list, &kv))
                {
                    JS_FreeValue(ctx, prop);
                    JS_FreeCString(ctx, key);
                    JS_FreeCString(ctx, value);
                    qrk_str_free(&kv.key);
                    qrk_str_free(&kv.value);
                    ret = JS_ThrowOutOfMemory(ctx);
                    goto fail;
                }

                JS_FreeCString(ctx, key);
                JS_FreeCString(ctx, value);
                JS_FreeValue(ctx, prop);

                JS_FreeAtom(ctx, ptab[i].atom);
            }
            js_free(ctx, ptab);
        }
        else
        {
            const char *value = JS_ToCString(ctx, init);
            if (!value)
            {
                ret = JS_EXCEPTION;
                goto fail;
            }
            qrk_rbuf_t value_str = {
                .base = value[0] == '?' ? (char *)value + 1 : (char *)value,
                .len = value[0] == '?' ? strlen(value) - 1 : strlen(value)
            };
            if (!qrk_url_form_urlencoded_parse(&value_str, &params->list, qrt->mctx))
            {
                JS_FreeCString(ctx, value);
                ret = JS_ThrowOutOfMemory(ctx);
                goto fail;
            }
        }
    }
    else
    {
        if (!qrk_buf_malloc(&params->list, qrt->mctx, sizeof(qrk_kv_t), 1)) {
            qrk_free(qrt->mctx, params);
            return JS_ThrowOutOfMemory(ctx);
        }
    }

    JSValue obj = JS_NewObjectClass(ctx, (int)qrk_qjs_urlsearchparams_class_id);

    if (JS_IsException(obj))
    {
        ret = JS_EXCEPTION;
        goto fail;
    }

    JS_SetOpaque(obj, params);

    return obj;
fail:
    if (params->list.base != NULL)
    {
        for (int i = 0; i < params->list.nmemb; i++)
        {
            qrk_kv_t * kv = (qrk_kv_t *)qrk_buf_get(&params->list, i);
            qrk_str_free(&kv->key);
            qrk_str_free(&kv->value);
        }
        qrk_buf_free(&params->list);
    }
    qrk_free(qrt->mctx, params);
    return ret;
}

static JSValue qrk_qjs_url_ctor (JSContext *ctx, JSValueConst new_target,
                                 int argc, JSValueConst *argv)
{
    if (argc < 1)
    {
        return JS_ThrowTypeError(ctx, "URL constructor requires at least one argument");
    }
    qrk_qjs_rt_t *qrt = JS_GetContextOpaque(ctx);
    qrk_url_t *base = NULL;
    qrk_qjs_url_ctx_t *url_ctx = qrk_malloc(qrt->mctx, sizeof(qrk_qjs_url_ctx_t));

    if (!url_ctx)
        return JS_ThrowOutOfMemory(ctx);

    url_ctx->url = NULL;
    url_ctx->ctx = ctx;

    if (unlikely(qrk_url_parser_init(&url_ctx->parser, qrt->mctx) != 0))
    {
        qrk_free(qrt->mctx, url_ctx);
        return JS_ThrowInternalError(ctx, "url_parser_init failed");
    }

    if (argc >= 2)
    {
        if (!JS_IsUndefined(argv[1]))
        {
            const char *base_url = JS_ToCString(ctx, argv[1]);
            if (!base_url) {
                qrk_free(qrt->mctx, url_ctx);
                return JS_EXCEPTION;
            }
            qrk_rbuf_t base_str = {
                    .base = (char *) base_url,
                    .len = strlen(base_url)
            };
            if (qrk_url_parse_basic(&url_ctx->parser, &base_str, NULL, &base, 0) != 0) {
                if (base)
                    qrk_url_free(base);
                JS_FreeCString(ctx, base_url);
                qrk_free(qrt->mctx, url_ctx);
                return JS_ThrowTypeError(ctx, "Failed to construct 'URL': Invalid base URL");
            };
            JS_FreeCString(ctx, base_url);
        }
    }

    const char *url_str = JS_ToCString(ctx, argv[0]);
    if (!url_str) {
        if (base)
            qrk_url_free(base);
        qrk_free(qrt->mctx, url_ctx);
        return JS_EXCEPTION;
    }
    qrk_rbuf_t url_str_buf = {
        .base = (char *) url_str,
        .len = strlen(url_str)
    };
    if (qrk_url_parse_basic(&url_ctx->parser, &url_str_buf, base, &url_ctx->url, 0) != 0) {
        if (base)
            qrk_url_free(base);
        if (url_ctx->url)
            qrk_url_free(url_ctx->url);
        JS_FreeCString(ctx, url_str);
        qrk_free(qrt->mctx, url_ctx);
        return JS_ThrowTypeError(ctx, "Failed to construct 'URL': Invalid URL");
    }
    JS_FreeCString(ctx, url_str);
    if (base)
        qrk_url_free(base);

    url_ctx->search_params = qrk_qjs_urlseachparams_init_url(ctx, (qrk_rbuf_t *)&url_ctx->url->query, url_ctx);
    if (JS_IsException(url_ctx->search_params))
        goto exception;

    JSValue obj = JS_NewObjectClass(ctx, (int)qrk_qjs_url_class_id);

    if (JS_IsException(obj))
        goto exception1;

    JS_SetOpaque(obj, url_ctx);

    return obj;
exception1:
    JS_FreeValue(ctx, url_ctx->search_params);
exception:
    qrk_url_free(url_ctx->url);
    qrk_free(qrt->mctx, url_ctx);
    return JS_EXCEPTION;
}

int qrk_qjs_url_init (JSContext *ctx)
{
    JSValue proto, obj;
    JSValue global = JS_GetGlobalObject(ctx);

    if (JS_IsException(global))
        return 1;

    /* URL class */
    JS_NewClassID(&qrk_qjs_url_class_id);
    JS_NewClass(JS_GetRuntime(ctx), qrk_qjs_url_class_id, &qrk_qjs_url_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, qrk_qjs_url_proto_funcs, countof(qrk_qjs_url_proto_funcs));

    obj = JS_NewCFunction2(ctx, qrk_qjs_url_ctor, "URL", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, qrk_qjs_url_class_id, proto);

    JS_SetPropertyStr(ctx, global, "URL", obj);
    // LegacyWindowAlias=webkitURL
    JS_SetPropertyStr(ctx, global, "webkitURL", JS_DupValue(ctx, obj));

    /* URLSearchParams class */
    JS_NewClassID(&qrk_qjs_urlsearchparams_class_id);
    JS_NewClass(JS_GetRuntime(ctx), qrk_qjs_urlsearchparams_class_id, &qrk_qjs_urlsearchparams_class);
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, qrk_qjs_urlsearchparams_proto_funcs, countof(qrk_qjs_urlsearchparams_proto_funcs));

    obj = JS_NewCFunction2(ctx, qrk_qjs_urlsearchparams_ctor, "URLSearchParams", 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, obj, proto);
    JS_SetClassProto(ctx, qrk_qjs_urlsearchparams_class_id, proto);

    JS_SetPropertyStr(ctx, global, "URLSearchParams", obj);
    JS_FreeValue(ctx, global);

    /* URLSearchParams Iterator Class */
    JS_NewClassID(&qrk_qjs_urlsearchparams_iterator_class_id);
    JS_NewClass(JS_GetRuntime(ctx), qrk_qjs_urlsearchparams_iterator_class_id, &qrk_qjs_urlsearchparams_iterator_class);

    JSValue iterator_proto = JS_GetIteratorPrototype(ctx);
    proto = JS_NewObjectProto(ctx, iterator_proto);
    JS_FreeValue(ctx, iterator_proto);
    JS_SetPropertyFunctionList(ctx, proto, qrk_qjs_urlsearchparams_iterator_proto_funcs, countof(qrk_qjs_urlsearchparams_iterator_proto_funcs));

    JS_SetClassProto(ctx, qrk_qjs_urlsearchparams_iterator_class_id, proto);
    return 0;
}
