#ifndef QRK_STD_SPOOKY_H
#define QRK_STD_SPOOKY_H

#include <quark/std/alloc.h>

/**
 * Spookyhash ported to C by Daniel Wykerd
 */

//
// SpookyHash: a 128-bit noncryptographic hash function
// By Bob Jenkins, public domain
//   Oct 31 2010: alpha, framework + SpookyHash::Mix appears right
//   Oct 31 2011: alpha again, Mix only good to 2^^69 but rest appears right
//   Dec 31 2011: beta, improved Mix, tested it for 2-bit deltas
//   Feb  2 2012: production, same bits as beta
//   Feb  5 2012: adjusted definitions of uint* to be more portable
//   Mar 30 2012: 3 bytes/cycle, not 4.  Alpha was 4 but wasn't thorough enough.
//   August 5 2012: SpookyV2 (different results)
// 
// Up to 3 bytes/cycle for long messages.  Reasonably fast for short messages.
// All 1 or 2 bit deltas achieve avalanche within 1% bias per output bit.
//
// This was developed for and tested on 64-bit x86-compatible processors.
// It assumes the processor is little-endian.  There is a macro
// controlling whether unaligned reads are allowed (by default they are).
// This should be an equally good hash on big-endian machines, but it will
// compute different results on them than on little-endian machines.
//
// Google's CityHash has similar specs to SpookyHash, and CityHash is faster
// on new Intel boxes.  MD4 and MD5 also have similar specs, but they are orders
// of magnitude slower.  CRCs are two or more times slower, but unlike 
// SpookyHash, they have nice math for combining the CRCs of pieces to form 
// the CRCs of wholes.  There are also cryptographic hashes, but those are even 
// slower than MD5.
//

// number of uint64's in internal state
#define sc_numVars 12

// size of the internal state
#define sc_blockSize (sc_numVars * 8)

// size of buffer of unhashed data, in bytes
#define sc_bufSize (2 * sc_blockSize)

//
// sc_const: a constant which:
//  * is not zero
//  * is odd
//  * is a not-very-regular mix of 1's and 0's
//  * does not need any other special mathematical properties
//
#define sc_const 0xdeadbeefdeadbeefLL

typedef struct qrk_spooky_ctx_s {
    uint64_t m_data[2*sc_numVars];   // unhashed data, for partial messages
    uint64_t m_state[sc_numVars];    // internal state of the hash
    size_t m_length;                 // total length of the input so far
    uint8_t  m_remainder;            // length of unhashed data stashed in m_data
} qrk_spooky_ctx_t;

//
// SpookyHash: hash a single message in one call, produce 128-bit output
//
void qrk_spooky_hash128 (
    const void *message,  // message to hash
    size_t length,        // length of message in bytes
    uint64_t *hash1,      // in/out: in seed 1, out hash value 1
    uint64_t *hash2       // in/out: in seed 2, out hash value 2
);

//
// Hash64: hash a single message in one call, return 64-bit output
//
uint64_t qrk_spooky_hash64 (
    const void *message,  // message to hash
    size_t length,        // length of message in bytes
    uint64_t seed);       // seed

//
// Hash32: hash a single message in one call, produce 32-bit output
//
uint32_t qrk_spooky_hash32 (
    const void *message,  // message to hash
    size_t length,        // length of message in bytes
    uint32_t seed);       // seed

//
// Init: initialize the context of a SpookyHash
//
void qrk_spooky_init (
    qrk_spooky_ctx_t *ctx,
    uint64_t seed1,       // any 64-bit value will do, including 0
    uint64_t seed2);      // different seeds produce independent hashes

//
// Update: add a piece of a message to a SpookyHash state
//
void qrk_spooky_update (
    qrk_spooky_ctx_t *ctx,
    const void *message,  // message fragment
    size_t length);       // length of message fragment in bytes

//
// Final: compute the hash for the current SpookyHash state
//
// This does not modify the state; you can keep updating it afterward
//
// The result is the same as if SpookyHash() had been called with
// all the pieces concatenated into one message.
//
void qrk_spooky_final (
    qrk_spooky_ctx_t *ctx,
    uint64_t *hash1,    // out only: first 64 bits of hash value.
    uint64_t *hash2);   // out only: second 64 bits of hash value.


#endif
