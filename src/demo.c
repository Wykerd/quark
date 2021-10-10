#include <quark/std/alloc.h>
#include <quark/std/buf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    qrk_malloc_ctx_t mctx;

    qrk_malloc_ctx_new(&mctx);

	qrk_buf_t buf;
	qrk_buf_malloc(&buf, &mctx, sizeof(int), 1);
	int number = 1;
	int *f = &number;
	int **ff = &f;
	qrk_buf_push_back(&buf, (const void **) ff, 1);
	*f = 2;
	qrk_buf_push_back(&buf, (const void **) ff, 1);
	*f = 3;
	qrk_buf_push_back(&buf, (const void **) ff, 1);
	*f = 4;
	qrk_buf_push_front(&buf, (const void **) ff, 1);

	printf("%d\n", *(int *)qrk_buf_get(&buf, 0));
	printf("%d\n", *(int *)qrk_buf_get(&buf, 1));
	printf("%d\n", *(int *)qrk_buf_get(&buf, 2));
	printf("%d\n", *(int *)qrk_buf_get(&buf, 3));

	qrk_buf_shift(&buf, 2);
	printf("ff %d\n", *(int *)qrk_buf_get(&buf, 0));
	printf("ff %d\n", *(int *)qrk_buf_get(&buf, 1));

    qrk_malloc_ctx_dump_leaks(&mctx);

    puts("DONE");
    return 0;
}
