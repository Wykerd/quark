#include <quark/std/err.h>
#include <quark/std/stream.h>
#include <uv.h>
#include <openssl/err.h>

static
const char *qrk__err_str_get_uv (qrk_err_t *err) {
	return uv_err_name(err->code);
}

static
const char *qrk__err_str_get_ssl (qrk_err_t *err) {
    return ERR_error_string(err->code, NULL);
}

const char *qrk_err_str_get_impl (qrk_err_t *err) {
	static const char *err_str[] = {
		"ok",
		"out of memory",
		"unreachable code reached"
	};
	return err_str[err->code];
}

const char *qrk_err_str_get_custom (qrk_err_str_getter **getters, qrk_err_origin_t range, qrk_err_t *err) {
	static const char *unknown = "unknown error";
	if (err->origin > range)
		return unknown;
	return getters[err->origin](err);
}

const char *qrk_err_str_get (qrk_err_t *err) {
	static qrk_err_str_getter *getters[] = {
		qrk_err_str_get_impl,
		qrk__err_str_get_uv,
        qrk__err_str_get_ssl,
	};
	return qrk_err_str_get_custom(getters, QRK_EO_CUSTOM - 1, err);
}

void qrk_err_emit(qrk_err_t *err)
{
    if (err->target_type >= QRK_ET_STREAM)
    {
        qrk_stream_t *stream = err->target;
        if (stream->on_error)
            return stream->on_error(stream, err);
    }
    fprintf(stderr, "error: %s\n", qrk_err_str_get(err));
    exit(err->origin);
}
