#include <quark/std/err.h>
#include <uv.h>

static
const char *qrk__err_str_get_uv (qrk_err_t *err) {
	return uv_err_name(err->code);
}

const char *qrk_err_str_get_impl (qrk_err_t *err) {
	static const char *err_str[] = {
		"ok",
		"out of memory"
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
		qrk__err_str_get_uv
	};
	return qrk_err_str_get_custom(getters, QRK_EO_CUSTOM - 1, err);
}
