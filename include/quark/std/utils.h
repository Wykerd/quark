#ifndef QRK_STD_H
#define QRK_STD_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#define STRINGIFY_DETAIL(x) #x
#define STRINGIFY(x) STRINGIFY_DETAIL(x)

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#if defined(__linux__)
#  include <endian.h>
#elif defined(__APPLE__)
#  include <machine/endian.h>
#  define be16toh(x) OSSwapBigToHostInt16(x)
#  define be32toh(x) OSSwapBigToHostInt32(x)
#  define be64toh(x) OSSwapBigToHostInt64(x)
#  define htobe16(x) OSSwapHostToBigInt16(x)
#  define htobe32(x) OSSwapHostToBigInt32(x)
#  define htobe64(x) OSSwapHostToBigInt64(x)
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#  define htobe16(x) htobe16(x)
#  define htobe32(x) htobe32(x)
#  define htobe64(x) htobe64(x)
#elif defined(_MSC_VER)
#  include <stdlib.h>
#  define be16toh(x) _byteswap_ushort(x)
#  define be32toh(x) _byteswap_ulong(x)
#  define be64toh(x) _byteswap_uint64(x)
#  define htobe16(x) _byteswap_ushort(x)
#  define htobe32(x) _byteswap_ulong(x)
#  define htobe64(x) _byteswap_uint64(x)
#endif

/**
 * Quickly check whether a string is utf-8
 */
const char *qrk_utf8_check(const char *s, size_t len);

#endif
