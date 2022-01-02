#include <quark/runtime/quickjs/vm.h>
#include <quark/runtime/quickjs/module.h>
#include <quark/runtime/quickjs/utils.h>

static void qrk__qjs_rt_stop (uv_async_t *handle)
{
    qrk_qjs_rt_t *rt = handle->data;

    uv_stop(rt->loop);
}

int qrk_qjs_rt_init(qrk_qjs_rt_t *rt, qrk_malloc_ctx_t *mctx, uv_loop_t *loop, int is_worker)
{
    rt->mctx = mctx;
    rt->loop = loop;
    rt->is_worker = is_worker;
    rt->flags = QRK_QJS_RT_FLAG_DEFAULT;

    if (uv_prepare_init(loop, &rt->handles.prepare))
        return 1;

    rt->handles.prepare.data = rt;

    if (uv_check_init(loop, &rt->handles.check))
        return 1;

    rt->handles.check.data = rt;

    if (uv_idle_init(loop, &rt->handles.idle))
        return 1;

    rt->handles.idle.data = rt;

    if (uv_async_init(loop, &rt->handles.stop, qrk__qjs_rt_stop))
        return 1;

    rt->handles.stop.data = rt;

    rt->rt = JS_NewRuntime2((const JSMallocFunctions *) &mctx->mf, NULL);

    if (rt->rt == NULL)
        return 1;

    rt->ctx = JS_NewContext(rt->rt);
    if (rt->ctx == NULL)
    {
        JS_FreeRuntime(rt->rt);
        return 1;
    }

    JS_SetRuntimeOpaque(rt->rt, rt);
    JS_SetContextOpaque(rt->ctx, rt);

    JS_AddIntrinsicBigFloat(rt->ctx);
    JS_AddIntrinsicBigDecimal(rt->ctx);
    JS_AddIntrinsicOperators(rt->ctx);
    JS_EnableBignumExt(rt->ctx, 1);

    JS_SetModuleLoaderFunc(rt->rt, NULL, qrk_qjs_module_loader, rt);

    // TODO: promise rejection handler
    // JS_SetPromiseRejectionTracker(rt->rt, qrk_qjs_promise_rejection_handler, NULL);

    return 0;
}

static void qrk__qjs_idle_noop (uv_idle_t *handle)
{
    // we just need the idle to be running for the loop to perform zero timeout polls
}

static void qrk__qjs_rt_no_block (qrk_qjs_rt_t *rt)
{
    if (JS_IsJobPending(rt->rt))
        uv_idle_start(&rt->handles.idle, qrk__qjs_idle_noop);
    else
        // when no jobs are pending, we can stop the idle handle
        // this will cause the loop to exit if there are no other active handles
        uv_idle_stop(&rt->handles.idle);
}

void qrk__qjs_prepare (uv_prepare_t *handle)
{
    qrk_qjs_rt_t *rt = handle->data;

    // check if there are still jobs pending
    qrk__qjs_rt_no_block(rt);
}

void qrk_qjs_rt_execute_jobs (qrk_qjs_rt_t *rt)
{
    JSContext *ctx = rt->ctx;
    JSContext *ctx1;
    int err;

    /* execute the pending jobs */
    for (;;) {
        err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
        if (err <= 0) {
            if (err < 0)
                qrk_qjs_dump_error (ctx1);
            break;
        }
    }
}

void qrk__qjs_check (uv_check_t *handle)
{
    qrk_qjs_rt_t *rt = handle->data;

    // run pending jobs that have been processed by the polling
    qrk_qjs_rt_execute_jobs(rt);

    // check if there are still jobs pending
    qrk__qjs_rt_no_block(rt);
}

void qrk_qjs_rt_loop (qrk_qjs_rt_t *rt)
{
    uv_prepare_start(&rt->handles.prepare, qrk__qjs_prepare);
    uv_check_start(&rt->handles.check, qrk__qjs_check);
    // unref handles to avoid keeping the loop alive after idle is stopped
    uv_unref((uv_handle_t *) &rt->handles.prepare);
    uv_unref((uv_handle_t *) &rt->handles.check);

    // we need to keep the async handle referenced for workers
    // to ensure that the loop doesn't exit
    if (!rt->is_worker)
        uv_unref((uv_handle_t *) &rt->handles.stop);

    // we're using the idle handle to make sure the loop uses non-blocking polling
    qrk__qjs_rt_no_block(rt);

    // start up the loop
    uv_run(rt->loop, UV_RUN_DEFAULT);
}

int qrk_qjs_rt_free(qrk_qjs_rt_t *rt)
{
    uv_close((uv_handle_t *) &rt->handles.stop, NULL);
    uv_close((uv_handle_t *) &rt->handles.idle, NULL);
    uv_close((uv_handle_t *) &rt->handles.check, NULL);
    uv_close((uv_handle_t *) &rt->handles.prepare, NULL);

    JS_FreeContext(rt->ctx);
    JS_FreeRuntime(rt->rt);

    // cleanup the loop
    int closed = 0;
    for (int i = 0; i < 6; i++) {
        if (uv_loop_close(rt->loop) == 0) {
            closed = 1;
            break;
        }
        uv_run(rt->loop, UV_RUN_NOWAIT);
    }

    return !closed;
}

int qrk_qjs_eval_buf (qrk_qjs_rt_t *rt, const qrk_rbuf_t *script, const char *script_name, int eval_flags)
{
    JSValue val;
    int ret;

    if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set
           import.meta */
        val = JS_Eval(rt->ctx, script->base, script->len, script_name,
                      eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            qrk_qjs_js_module_set_import_meta(rt->ctx, val, 1, 1);
            val = JS_EvalFunction(rt->ctx, val);
        }
    } else {
        val = JS_Eval(rt->ctx, script->base, script->len, script_name, eval_flags);
    }
    if (JS_IsException(val)) {
        qrk_qjs_dump_error(rt->ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(rt->ctx, val);
    return ret;
}

int qrk_qjs_emit_load_event (qrk_qjs_rt_t *rt)
{
    int ret;

    JSValue global = JS_GetGlobalObject(rt->ctx);
    if (JS_IsUndefined(global))
    {
        ret = 1;
        goto fail0;
    }
    JSValue dispatch_func = JS_GetPropertyStr(rt->ctx, global, "dispatchEvent");
    if (!JS_IsFunction(rt->ctx, dispatch_func)) {
        ret = 1;
        goto fail1;
    }
    JSValue event_ctor = JS_GetPropertyStr(rt->ctx, global, "Event");
    if (!JS_IsFunction(rt->ctx, event_ctor)) {
        ret = 1;
        goto fail2;
    }
    JSValue event_name = JS_NewString(rt->ctx, "load");
    if (JS_IsException(event_name)) {
        ret = 1;
        goto fail3;
    }
    JSValueConst args[] = { event_name };
    JSValue event = JS_CallConstructor(rt->ctx, event_ctor, 1, args);
    if (JS_IsException(event)) {
        ret = 1;
        goto fail4;
    }
    JSValue fret = JS_Call(rt->ctx, dispatch_func, global, 1, &event);
    if (JS_IsException(fret)) {
        ret = 1;
        goto fail5;
    }

    ret = 0;

fail5:
    JS_FreeValue(rt->ctx, fret);
fail4:
    JS_FreeValue(rt->ctx, event);
fail3:
    JS_FreeValue(rt->ctx, event_name);
fail2:
    JS_FreeValue(rt->ctx, event_ctor);
fail1:
    JS_FreeValue(rt->ctx, dispatch_func);
fail0:
    JS_FreeValue(rt->ctx, global);
    return ret;
}

int qrk_qjs_eval_file (qrk_qjs_rt_t *rt, const char *filename, int is_module, int is_main, const char *script_name)
{
    uint8_t *buf;
    size_t buf_len;
    int ret, eval_flags;

    buf = qrk_qjs_load_file(rt->ctx, &buf_len, filename);

    if (buf == NULL)
        return 1;

    if (is_module == -1)
        eval_flags = JS_DetectModule((char *)buf, buf_len) ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL;
    else if (is_module)
        eval_flags = JS_EVAL_TYPE_MODULE;
    else
        eval_flags = JS_EVAL_TYPE_GLOBAL;

    qrk_rbuf_t str = {
        .base = (char *)buf,
        .len = buf_len
    };

    ret = qrk_qjs_eval_buf(rt, &str, script_name != NULL ? script_name : filename, eval_flags);

    if (ret == 0 && is_main)
    {
        qrk_qjs_emit_load_event(rt);
    }

    js_free(rt->ctx, buf);
    return ret;
}
