#ifndef QRK_STD_STREAM_H
#define QRK_STD_STREAM_H

#include <quark/std/alloc.h>
#include <quark/std/err.h>
#include <quark/std/buf.h>
#include <uv.h>

typedef struct qrk_stream_s qrk_stream_t;

typedef void (*qrk_stream_err_cb)(qrk_stream_t *stream, qrk_err_t *err);
typedef void (*qrk_stream_read_cb)(qrk_stream_t *stream, qrk_rbuf_t *buf);
typedef void (*qrk_stream_cb)(qrk_stream_t *stream);
typedef void (*qrk_stream_write)(qrk_stream_t *stream, qrk_rbuf_t *buf);

#define QRK_LOOP_HANDLE_FIELD 		\
	uv_loop_t *loop;

#define QRK_STREAM_FIELDS 			\
	qrk_stream_err_cb on_error;		\
    qrk_stream_cb on_write;         \
	qrk_stream_read_cb on_read; 	\
	qrk_stream_write write;

struct qrk_stream_s {
	QRK_MEMORY_CONTEXT_FIELDS
	QRK_ERR_FIELDS
	QRK_LOOP_HANDLE_FIELD
	QRK_STREAM_FIELDS
};

typedef struct qrk_loop_s {
	QRK_MEMORY_CONTEXT_FIELDS
	QRK_ERR_FIELDS
	QRK_LOOP_HANDLE_FIELD
} qrk_loop_t;

#endif
