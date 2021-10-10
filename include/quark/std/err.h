#ifndef QRK_STD_ERR_H
#define QRK_STD_ERR_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

typedef enum qrk_err_target {
	QRK_ET_DNS = 1,
	QRK_ET_TCP,
	QRK_ET_TLS,
	QRK_ET_WS,
	QRK_ET_CUSTOM // User defined classes should use target values greater than this
} qrk_err_target_t;

typedef struct qrk_err_s {
	ssize_t code;
	ssize_t origin;
	qrk_err_target_t target_type;
	void *target;
} qrk_err_t;

typedef enum qrk_err_impl {
	QRK_E_OK = 0,
	QRK_E_OOM
} qrk_err_impl_t;

typedef enum qrk_err_origin {
	QRK_EO_IMPL = 0,
	QRK_EO_UV,
	QRK_EO_CUSTOM
} qrk_err_origin_t;

typedef const char *(qrk_err_str_getter)(qrk_err_t *err);

#define QRK_ERR_FIELDS \
	qrk_err_target_t type;

const char *qrk_err_str_get_impl (qrk_err_t *err);
const char *qrk_err_str_get_custom (qrk_err_str_getter **getters, qrk_err_origin_t range, qrk_err_t *err);
const char *qrk_err_str_get (qrk_err_t *err);

#endif
