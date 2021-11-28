#ifndef QRK_NET_TLS_H
#define QRK_NET_TLS_H

#include <quark/std/stream.h>

typedef struct qrk_tls_s qrk_tls_t;

struct qrk_tls_s {
    QRK_MEMORY_CONTEXT_FIELDS
    QRK_ERR_FIELDS
    QRK_LOOP_HANDLE_FIELD
    QRK_STREAM_FIELDS
};



#endif //QRK_NET_TLS_H
