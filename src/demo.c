#include <quark/std/alloc.h>
#include <quark/std/stream.h>
#include <quark/net/tcp.h>
#include <quark/std/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <quark/net/tls.h>
#include <quark/url/url.h>

int main(int argc, char const *argv[])
{
    qrk_malloc_ctx_t mctx;
    qrk_malloc_ctx_new(&mctx);

    qrk_url_parser_t parser;
    qrk_url_parser_init(&parser, &mctx);

    qrk_url_t *url = NULL;

    qrk_rbuf_t uri = {
        .base = "blob:https://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f",
        .len = strlen("blob:https://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f")
    };

    int rrr = qrk_url_parse_basic(&parser, &uri, NULL, &url, 0);

    for (int i = 0; i < url->path.nmemb; i++)
    {
        qrk_str_t * str = (qrk_str_t *)qrk_buf_get(&url->path, i);
        printf("%.*s\n", str->len, str->base);
    }

    printf("%.*s\n", url->host.str.len, url->host.str.base);

    qrk_str_t str;
    qrk_str_malloc(&str, &mctx, 1);

    qrk_url_serialize(url, &str, 0);

    printf("%.*s\n", str.len, str.base);


    qrk_html_origin_tuple_t *origin = NULL;

    int rrr1 = qrk_url_origin(url, &origin, &mctx);

    str.len = 0;

    qrk_html_origin_serialize(&str, origin);

    printf("%.*s\n", str.len, str.base);

    qrk_str_free(&str);

    qrk_html_origin_tuple_free(origin);
    qrk_free(&mctx, origin);

    qrk_url_free(url);
    qrk_free(&mctx, url);

    qrk_buf_t list;

    qrk_rbuf_t encoded = {
            .base = "a=hel%70lo+world&b==",
            .len = strlen("a=hel%7=world==0lo&b")
    };

    int rr2 = qrk_url_form_urlencoded_parse(&encoded, &list, &mctx);

    for (int i = 0; i < list.nmemb; i++)
    {
        qrk_kv_t * kv = (qrk_kv_t *)qrk_buf_get(&list, i);
        printf("%.*s -> %.*s\n", kv->key.len, kv->key.base, kv->value.len, kv->value.base);
        qrk_str_free(&kv->key);
        qrk_str_free(&kv->value);
    }
    qrk_buf_free(&list);

    qrk_malloc_ctx_dump_leaks(&mctx);
    puts("DONE");
    return 0;
}
