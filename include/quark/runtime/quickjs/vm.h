#ifndef QRK_RUNTIME_QJS_VM_H
#define QRK_RUNTIME_QJS_VM_H

#include <quark/std/alloc.h>
#include <quickjs.h>
#include <uv.h>
#include <quark/std/buf.h>

typedef enum qrk_qjs_rt_flags {
    QRK_QJS_RT_FLAG_NONE                = 0,
    QRK_QJS_RT_FLAG_NATIVE_MODULES      = 1 << 0,
    QRK_QJS_RT_FLAG_INET_MODULES        = 1 << 1,
    QRK_QJS_RT_FLAG_JSON_MODULES        = 1 << 2,
    QRK_QJS_RT_FLAG_FS_MODULES          = 1 << 3,
} qrk_qjs_rt_flags_t;

#define QRK_QJS_RT_FLAG_DEFAULT (QRK_QJS_RT_FLAG_NATIVE_MODULES | \
                                 QRK_QJS_RT_FLAG_INET_MODULES | \
                                 QRK_QJS_RT_FLAG_JSON_MODULES | \
                                 QRK_QJS_RT_FLAG_FS_MODULES)

typedef struct qrk_qjs_rt_s {
    JSRuntime *rt;
    JSContext *ctx;
    // XXX: quark libraries and qjs use different memory states
    qrk_malloc_ctx_t *mctx;
    uv_loop_t *loop;
    qrk_qjs_rt_flags_t flags;
    struct {
        uv_prepare_t prepare;
        uv_check_t check;
        uv_idle_t idle;
        uv_async_t stop;
    } handles;
    int is_worker;
} qrk_qjs_rt_t;

int qrk_qjs_rt_init(qrk_qjs_rt_t *rt, qrk_malloc_ctx_t *mctx, uv_loop_t *loop, int is_worker);
void qrk_qjs_rt_loop(qrk_qjs_rt_t *rt);
/**
 * Frees and cleans up the runtime.
 * @param rt Runtime to free.
 * @return Should always return 0. If it returns non-zero, the loop did not close properly.
 */
int qrk_qjs_rt_free(qrk_qjs_rt_t *rt);
void qrk_qjs_rt_execute_jobs (qrk_qjs_rt_t *rt);

int qrk_qjs_eval_buf (qrk_qjs_rt_t *rt, const qrk_rbuf_t *script, const char *script_name, int eval_flags);
int qrk_qjs_eval_file (qrk_qjs_rt_t *rt, const char *filename, int is_module, int is_main, const char *script_name);
int qrk_qjs_emit_load_event (qrk_qjs_rt_t *rt);


#endif //QRK_RUNTIME_QJS_VM_H
