#ifndef QRK_RUNTIME_QJS_MODULE_H
#define QRK_RUNTIME_QJS_MODULE_H

#include <quickjs.h>

typedef JSModuleDef *(qrk_qjs_module_init_func)(JSContext *ctx, const char *module_name);

int qrk_qjs_js_module_set_import_meta (JSContext *ctx, JSValueConst func_val,
                                       JS_BOOL use_realpath, JS_BOOL is_main);

JSModuleDef *qrk_qjs_module_loader_fs (JSContext *ctx, const char *module_name);

JSModuleDef *qrk_qjs_module_loader_json (JSContext *ctx, const char *module_name);

JSModuleDef *qrk_qjs_module_loader_native (JSContext *ctx, const char *module_name);

JSModuleDef *qrk_qjs_module_loader (JSContext *ctx, const char *module_name,
                                    void *opaque);

// TODO: cleaner file loading. this should be moved to a separate file.
// maybe we can add a fs module to quark with both sync and async api.
uint8_t *qrk_qjs_load_file (JSContext *ctx, size_t *pbuf_len, const char *filename);

#endif