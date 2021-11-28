#ifndef QRK_STD_ERR_H
#define QRK_STD_ERR_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

typedef enum qrk_target {
    QRK_ET_UNKNOWN = 0,
	QRK_ET_DNS,
    QRK_ET_LOOP,
    QRK_ET_STREAM,
	QRK_ET_TCP,
	QRK_ET_TLS,
	QRK_ET_WS,
	QRK_ET_CUSTOM
    // Error targets > QRK_ET_CUSTOM are reserved for custom error targets.
} qrk_target_t;

typedef struct qrk_err_s {
	ssize_t code;
	ssize_t origin;
	qrk_target_t target_type;
	void *target;
} qrk_err_t;

typedef enum qrk_err_impl {
	QRK_E_OK = 0,
	QRK_E_OOM,
	QRK_E_UNREACHABLE
} qrk_err_impl_t;

typedef enum qrk_err_origin {
	QRK_EO_IMPL = 0, // Error originated in implementation
    // Error origins > QRK_EO_IMPL originated from library code
	QRK_EO_UV,
    QRK_EO_OPENSSL,
	QRK_EO_CUSTOM
    // Error origins > QRK_EO_CUSTOM reserved for user defined error origins
} qrk_err_origin_t;

typedef const char *(qrk_err_str_getter)(qrk_err_t *err);

#define QRK_ERR_FIELDS \
	qrk_target_t type;

const char *qrk_err_str_get_impl (qrk_err_t *err);
const char *qrk_err_str_get_custom (qrk_err_str_getter **getters, qrk_err_origin_t range, qrk_err_t *err);
const char *qrk_err_str_get (qrk_err_t *err);
void qrk_err_emit(qrk_err_t *err);

#endif
