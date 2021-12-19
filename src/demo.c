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
        .base = "https://www.google.com/hello/world?hello=world",
        .len = 46
    };

    int rrr = qrk_url_parse_basic(&parser, &uri, NULL, &url, 0);

    for (int i = 0; i < url->path.nmemb; i++)
    {
        qrk_str_t * str = (qrk_str_t *)qrk_buf_get(&url->path, i);
        printf("%.*s\n", str->len, str->base);
    }

    qrk_malloc_ctx_dump_leaks(&mctx);
    puts("DONE");
    return 0;
}
