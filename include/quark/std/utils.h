#ifndef QRK_STD_H
#define QRK_STD_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#define STRINGIFY_DETAIL(x) #x
#define STRINGIFY(x) STRINGIFY_DETAIL(x)

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

/**
 * Quickly check whether a string is utf-8
 */
const char *qrk_utf8_check(const char *s, size_t len);

#endif
