#if !defined(_MD5_H_INCLUDED_)
#define _MD5_H_INCLUDED_
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to md5_init, call md5_update as
 * needed on buffers full of bytes, and then call md5_final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * Changed so as no longer to depend on Colin Plumb's `usual.h' header
 * definitions; now uses stuff from dpkg's config.h.
 *  - Ian Jackson <ijackson@nyx.cs.du.edu>.
 * Still in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/** @brief structure of a MD5 context */
typedef struct {
	/** @brief the current state */
	uint32_t buf[4];

	/** @brief total number of bytes count */
	uint32_t bytes[2];

	/** @brief current input block */
	uint32_t in[16];

}	md5_context_t;

/**
 * @brief initialize a md5 context
 *
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 *
 * @param ctx pointer to a md5_context_t
 */
extern void md5_init(md5_context_t *ctx);

/**
 * @brief update a md5 context with a buffer
 *
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 *
 * @param ctx pointer to a md5_context_t
 * @param buf pointer to an array of bytes
 * @param len number of bytes in array
 */
extern void md5_update(md5_context_t *ctx, uint8_t const *buf, unsigned len);

/**
 * @brief finalize a md5 context and return the digest
 *
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 *
 * @param digest array of 16 bytes to receive the MD5 digest
 * @param ctx the md5 context to finish
 */
extern void md5_final(uint8_t digest[16], md5_context_t *ctx);

/**
 * @brief return the md5 digest string for an array of bytes
 *
 * @param buff array of bytes to hash
 * @param size number of bytes in the buffer
 * @result returns a pointer to a string containing the MD5 digest in hex
 */
extern const char *md5_digest(void *buff, size_t size);

/**
 * @brief test the MD5 digest with known results
 *
 * @result returns 0 on success, -1 on error
 */
extern int md5_test(void);

#endif	/* !defined(_MD5_H_INCLUDED_) */
