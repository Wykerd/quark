#include <quark/runtime/quickjs/utils.h>

// TODO: @console printer
static void qrk__qjs_dump_obj (JSContext *ctx, FILE *f, JSValueConst val)
{
    const char *str;

    str = JS_ToCString(ctx, val);
    if (str) {
        fprintf(f, "%s\n", str);
        JS_FreeCString(ctx, str);
    } else {
        fprintf(f, "[exception]\n");
    }
}

static void qrk__qjs_dump_error (JSContext *ctx, JSValueConst exception_val)
{
    JSValue val;
    int is_error;

    is_error = JS_IsError(ctx, exception_val);
    qrk__qjs_dump_obj(ctx, stderr, exception_val);
    if (is_error) {
        val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (!JS_IsUndefined(val)) {
            qrk__qjs_dump_obj(ctx, stderr, val);
        }
        JS_FreeValue(ctx, val);
    }
}

void qrk_qjs_dump_error (JSContext *ctx)
{
    JSValue exception_val;

    exception_val = JS_GetException(ctx);
    qrk__qjs_dump_error(ctx, exception_val);
    JS_FreeValue(ctx, exception_val);
}