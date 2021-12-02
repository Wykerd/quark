#ifndef QRK_STD_H
#define QRK_STD_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <quark/std/buf.h>

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

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

/**
 * Decode UTF-8 code points
 * @author Bjoern Hoehrmann
 * @copyright http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 * @example
 * void
 * printCodePoints(uint8_t* s) {
 *  uint32_t codepoint;
 *  uint32_t state = 0;
 *
 *  for (; *s; ++s)
 *    if (!decode(&state, &codepoint, *s))
 *      printf("U+%04X\n", codepoint);

 *  if (state != UTF8_ACCEPT)
 *    printf("The string is not well-formed\n");
 *
 * }
 * @param state
 * @param codep
 * @param byte
 * @return returns the value of the next state (UTF8_ACCEPT or UTF8_REJECT)
 */
uint32_t qrk_utf8_decode (uint32_t* state, uint32_t* codep, uint32_t byte);

int qrk_utf8_isascii (const char *s);

int qrk_infra_split_strict_static (qrk_rbuf_t *string, uint32_t delimiter, size_t *tokens, size_t max_tokens);

#endif
