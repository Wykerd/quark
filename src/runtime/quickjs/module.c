#include <quark/runtime/quickjs/module.h>
#include <quark/runtime/quickjs/vm.h>
#include <quark/std/buf.h>
#include <quark/url/url.h>
#include <string.h>
#include <stdlib.h>

uint8_t *qrk_qjs_load_file (JSContext *ctx, size_t *pbuf_len, const char *filename)
{
    FILE *f;
    uint8_t *buf = NULL;
    size_t buf_len;
    long lret;

    f = fopen(filename, "rb");
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) < 0)
        goto fail;
    lret = ftell(f);
    if (lret < 0)
        goto fail;
    /* XXX: on Linux, ftell() return LONG_MAX for directories */
    if (lret == LONG_MAX) {
        errno = EISDIR;
        goto fail;
    }
    buf_len = lret;
    if (fseek(f, 0, SEEK_SET) < 0)
        goto fail;
    if (ctx)
        buf = js_malloc(ctx, buf_len + 1);
    else
        buf = malloc(buf_len + 1);
    if (!buf)
        goto fail;
    if (fread(buf, 1, buf_len, f) != buf_len) {
        errno = EIO;
        if (ctx)
            js_free(ctx, buf);
        else
            free(buf);
        fail:
        fclose(f);
        return NULL;
    }
    buf[buf_len] = '\0';
    fclose(f);
    *pbuf_len = buf_len;
    return buf;
}

#include <quark/no_malloc.h>
#include <assert.h>
#include "../../../deps/quickjs/src/cutils.h"

int qrk_qjs_js_module_set_import_meta (JSContext *ctx, JSValueConst func_val,
                                       JS_BOOL use_realpath, JS_BOOL is_main)
{
    JSModuleDef *m;
    char buf[PATH_MAX + 16];
    JSValue meta_obj;
    JSAtom module_name_atom;
    const char *module_name;

    assert(JS_VALUE_GET_TAG(func_val) == JS_TAG_MODULE);
    m = JS_VALUE_GET_PTR(func_val);

    module_name_atom = JS_GetModuleName(ctx, m);
    module_name = JS_AtomToCString(ctx, module_name_atom);
    JS_FreeAtom(ctx, module_name_atom);
    if (!module_name)
        return -1;
    if (!strchr(module_name, ':')) {
        strcpy(buf, "file://");
        /* realpath() cannot be used with modules compiled with qjsc
           because the corresponding module source code is not
           necessarily present */
        if (use_realpath) {
            uv_fs_t req;
            int r = uv_fs_realpath(NULL, &req, module_name, NULL);
            if (r != 0) {
                uv_fs_req_cleanup(&req);
                JS_ThrowTypeError(ctx, "realpath failure");
                JS_FreeCString(ctx, module_name);
                return -1;
            }
            pstrcat(buf, sizeof(buf), req.ptr);
            uv_fs_req_cleanup(&req);
        } else
        {
            pstrcat(buf, sizeof(buf), module_name);
        }
    } else {
        pstrcpy(buf, sizeof(buf), module_name);
    }
    JS_FreeCString(ctx, module_name);

    meta_obj = JS_GetImportMeta(ctx, m);
    if (JS_IsException(meta_obj))
        return -1;
    JS_DefinePropertyValueStr(ctx, meta_obj, "url",
                              JS_NewString(ctx, buf),
                              JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, meta_obj, "main",
                              JS_NewBool(ctx, is_main),
                              JS_PROP_C_W_E);
    JS_FreeValue(ctx, meta_obj);
    return 0;
}

JSModuleDef *qrk_qjs_module_loader_fs (JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    size_t buf_len;
    uint8_t *buf;
    JSValue func_val;

    buf = qrk_qjs_load_file(ctx, &buf_len, module_name);
    if (!buf) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s'",
                               module_name);
        return NULL;
    }

    /* compile the module */
    func_val = JS_Eval(ctx, (char *)buf, buf_len, module_name,
                       JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    js_free(ctx, buf);
    if (JS_IsException(func_val))
        return NULL;
    /* XXX: could propagate the exception */
    qrk_qjs_js_module_set_import_meta(ctx, func_val, TRUE, FALSE);
    /* the module is already referenced, so we must free it */
    m = JS_VALUE_GET_PTR(func_val);
    JS_FreeValue(ctx, func_val);
    return m;
}

JSModuleDef *qrk_qjs_module_loader_json (JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    size_t buf_len;
    uint8_t *buf;
    JSValue func_val;

    buf = qrk_qjs_load_file(ctx, &buf_len, module_name);
    if (!buf) {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s'",
                               module_name);
        return NULL;
    }

    qrk_qjs_rt_t *rt = JS_GetContextOpaque(ctx);

    qrk_str_t module_json;

    if (!qrk_str_malloc(&module_json, rt->mctx, buf_len + sizeof("export default JSON.parse(``)"))) {
fail_oom:
        js_free(ctx, buf);
        return NULL;
    }

    if (!qrk_str_push_back(&module_json, "export default JSON.parse(`", 27))
    {
fail_json:
        qrk_str_free(&module_json);
        goto fail_oom;
    }

    for (size_t i = 0; i < buf_len; i++)
    {
        if (buf[i] == '`')
        {
            if (!qrk_str_push_back(&module_json, "\\`", 2))
                goto fail_json;
        }
        else
        {
            if (!qrk_str_push_back(&module_json, (char *)&buf[i], 1))
                goto fail_json;
        }
    }

    if (!qrk_str_push_back(&module_json, "`)", 2))
        goto fail_json;

    /* compile the module */
    func_val = JS_Eval(ctx, (char *)buf, buf_len, module_name,
                       JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    js_free(ctx, buf);
    qrk_str_free(&module_json);
    if (JS_IsException(func_val))
        return NULL;
    /* XXX: could propagate the exception */
    qrk_qjs_js_module_set_import_meta(ctx, func_val, TRUE, FALSE);
    /* the module is already referenced, so we must free it */
    m = JS_VALUE_GET_PTR(func_val);
    JS_FreeValue(ctx, func_val);
    return m;
}

JSModuleDef *qrk_qjs_module_loader_native (JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    uv_lib_t hd;
    qrk_qjs_module_init_func *init;
    char *filename;

    if (!strchr(module_name, '/')) {
        /* must add a '/' so that the DLL is not searched in the
           system library paths */
        filename = js_malloc(ctx, strlen(module_name) + 2 + 1);
        if (!filename)
            return NULL;
        strcpy(filename, "./");
        strcpy(filename + 2, module_name);
    } else {
        filename = (char *)module_name;
    }

    int r = uv_dlopen(filename, &hd);

    if (filename != module_name)
        js_free(ctx, filename);

    if (r != 0)
    {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s' as native module",
                               module_name);

        return NULL;
    }

    r = uv_dlsym(&hd, "qjs_module_init", (void **)&init);

    if (r != 0)
    {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': qjs_module_init not found",
                               module_name);
fail:
        uv_dlclose(&hd);
        return NULL;
    }

    m = init(ctx, module_name);
    if (!m)
    {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': initialization failed",
                               module_name);
        goto fail;
    }

    return m;
}

JSModuleDef *qrk_qjs_module_loader (JSContext *ctx, const char *module_name,
                                    void *opaque)
{
    JSModuleDef *m = NULL;
    static const char http[] = "http:";
    static const char https[] = "https:";
    static const char *native[] = {
        ".dylib",
        ".so",
        ".dll"
    };
    static const int native_len[] = {
        6,
        3,
        4
    };

    qrk_rbuf_t module_name_buf = {
        .base = (char *)module_name,
        .len = strlen(module_name)
    };

    qrk_qjs_rt_t *rt = JS_GetContextOpaque(ctx);

    if ((rt->flags & QRK_QJS_RT_FLAG_INET_MODULES))
    {
        if ((module_name_buf.len > 5 && !strncasecmp(module_name, http, 5)) ||
            (module_name_buf.len > 6 && !strncasecmp(module_name, https, 6)))
        {
            qrk_url_t *url = NULL;
            qrk_url_parser_t parser;

            if (!qrk_url_parser_init(&parser, rt->mctx))
                goto fs_fallback;

            if (!qrk_url_parse_basic(&parser, &module_name_buf, NULL, &url, QRK_URL_PARSER_STATE_NO_OVERRIDE))
            {
                if (url != NULL)
                    qrk_url_free(url);
                goto fs_fallback;
            }

            qrk_str_t content;

            JS_ThrowReferenceError(ctx, "network module loading not implemented");

            if (url != NULL)
                qrk_url_free(url);

            return NULL;
        }
    }

    if ((rt->flags & QRK_QJS_RT_FLAG_NATIVE_MODULES))
    {
        for (int i = 0; i < 3; i++)
        {
            if (module_name_buf.len > native_len[i] &&
                !strncasecmp(module_name + module_name_buf.len - native_len[i], native[i], native_len[i]))
            {
                return qrk_qjs_module_loader_native(ctx, module_name);
            }
        }
    }

fs_fallback:
    if ((rt->flags & QRK_QJS_RT_FLAG_JSON_MODULES))
    {
        return qrk_qjs_module_loader_json(ctx, module_name);
    }

    if ((rt->flags & QRK_QJS_RT_FLAG_FS_MODULES))
        m = qrk_qjs_module_loader_fs(ctx, module_name);
    else
    {
        JS_ThrowReferenceError(ctx, "could not load module filename '%s': no module loader available",
                               module_name);
    }

    return m;
}