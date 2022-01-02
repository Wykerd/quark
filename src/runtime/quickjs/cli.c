#include <quark/runtime/quickjs/vm.h>
#include <quark/runtime/quickjs/mod/url.h>
#include <string.h>

JSValue qrk_qjs_print (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    int i;
    const char *printval = JS_ToCString(ctx, argv[0]);
    puts(printval);
    JS_FreeCString(ctx, printval);
    return JS_UNDEFINED;
}

int main (int argc, char **argv)
{
    qrk_qjs_rt_t rt;
    qrk_malloc_ctx_t mctx;
    qrk_malloc_ctx_new(&mctx);

    if (qrk_qjs_rt_init(&rt, &mctx, uv_default_loop(), 0))
        return 1;

    qrk_rbuf_t script = {
        .base = "let url = new URL('https://netflix.com/watch/1234?trackid=4321'); "
                "url.protocol = 'http'; "
                "url.hash = 'daniel'; "
                "url.search = '?testid=4321'; "
                "url.pathname = '/test/path'; "
                "print(url.toString()); "
                "url.host = '192.168.1.252'; "
                "print(url.origin); "
                "print(JSON.stringify([...url.searchParams]));",
    };

    script.len = strlen(script.base);

    JSValue printfunc = JS_NewCFunction(rt.ctx, qrk_qjs_print, "print", 1);
    JSValue global = JS_GetGlobalObject(rt.ctx);
    JS_SetPropertyStr(rt.ctx, global, "print", printfunc);
    JS_FreeValue(rt.ctx, global);

    qrk_qjs_url_init(rt.ctx);

    qrk_qjs_eval_buf(&rt, &script, "<eval>", JS_EVAL_TYPE_GLOBAL);

    qrk_qjs_rt_loop(&rt);

    qrk_qjs_rt_free(&rt);

    qrk_malloc_ctx_free(&mctx);

    qrk_malloc_ctx_dump_leaks(&mctx);
}