#include <quark/std/alloc.h>
#include <quark/std/buf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    qrk_malloc_ctx_t mctx;

    qrk_malloc_ctx_new(&mctx);

    void *ptr = qrk_malloc(&mctx, 100);
    qrk_malloc(&mctx, 100);
    qrk_malloc(&mctx, 100);

    qrk_free(&mctx, ptr);

    qrk_malloc_ctx_dump_leaks(&mctx);

    puts("DONE");
    return 0;
}
