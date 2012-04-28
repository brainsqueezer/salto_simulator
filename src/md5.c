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

#include <SDL.h>
#include "md5.h"

#if	SDL_BYTEORDER == SDL_BIG_ENDIAN
void byte_swap(uint32_t *buf, unsigned words)
{
	uint8_t *p = (uint8_t *)buf;

	while (words--) {
		*buf++ = (uint32_t)((unsigned)p[3] << 8 | p[2]) << 16 |
			((unsigned)p[1] << 8 | p[0]);
		p += 4;
	}
}
#elif	SDL_BYTEORDER == SDL_LIL_ENDIAN
#define byte_swap(buf,words)
#else
#error SDL byte order not defined
#endif

/**
 * @brief initialize a md5 context
 *
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 *
 * @param ctx pointer to a md5_context_t
 */
void md5_init(md5_context_t *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bytes[0] = 0;
	ctx->bytes[1] = 0;
}
/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
	 (w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/**
 * @brief transform an md5 context's buffer with a block
 *
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data. md5_update blocks
 * the data and converts bytes into longwords for this routine.
 *
 * @param buf pointer to a md5_context_t buf (4 words)
 * @param in pointer to an array of 16 words
 */
void md5_transform(uint32_t buf[4], uint32_t const in[16])
{
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[ 0] + 0xd76aa478,  7);
	MD5STEP(F1, d, a, b, c, in[ 1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[ 2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[ 3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[ 4] + 0xf57c0faf,  7);
	MD5STEP(F1, d, a, b, c, in[ 5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[ 6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[ 7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[ 8] + 0x698098d8,  7);
	MD5STEP(F1, d, a, b, c, in[ 9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122,  7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[ 1] + 0xf61e2562,  5);
	MD5STEP(F2, d, a, b, c, in[ 6] + 0xc040b340,  9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[ 0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[ 5] + 0xd62f105d,  5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453,  9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[ 4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[ 9] + 0x21e1cde6,  5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6,  9);
	MD5STEP(F2, c, d, a, b, in[ 3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[ 8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
	MD5STEP(F2, d, a, b, c, in[ 2] + 0xfcefa3f8,  9);
	MD5STEP(F2, c, d, a, b, in[ 7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[ 5] + 0xfffa3942,  4);
	MD5STEP(F3, d, a, b, c, in[ 8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[ 1] + 0xa4beea44,  4);
	MD5STEP(F3, d, a, b, c, in[ 4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[ 7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
	MD5STEP(F3, d, a, b, c, in[ 0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[ 3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[ 6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[ 9] + 0xd9d4d039,  4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[ 2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[ 0] + 0xf4292244,  6);
	MD5STEP(F4, d, a, b, c, in[ 7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[ 5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3,  6);
	MD5STEP(F4, d, a, b, c, in[ 3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[ 1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[ 8] + 0x6fa87e4f,  6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[ 6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[ 4] + 0xf7537e82,  6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[ 2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[ 9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

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
void md5_update(md5_context_t *ctx, uint8_t const *buf, unsigned len)
{
	uint32_t t;

	/* Update byte count */

	t = ctx->bytes[0];
	if ((ctx->bytes[0] = t + len) < t)
		ctx->bytes[1]++;	/* Carry from low to high */

	t = 64 - (t & 0x3f);	/* Space available in ctx->in (at least 1) */
	if (t > len) {
		memcpy((uint8_t *)ctx->in + 64 - t, buf, len);
		return;
	}
	/* First chunk is an odd size */
	memcpy((uint8_t *)ctx->in + 64 - t, buf, t);
	byte_swap(ctx->in, sizeof(ctx->in)/sizeof(ctx->in[0]));
	md5_transform(ctx->buf, ctx->in);
	buf += t;
	len -= t;

	/* Process data in 64-byte chunks */
	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byte_swap(ctx->in, sizeof(ctx->in)/sizeof(ctx->in[0]));
		md5_transform(ctx->buf, ctx->in);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */
	memcpy(ctx->in, buf, len);
}

/**
 * @brief finalize a md5 context and return the digest
 *
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 *
 * @param digest array of 16 bytes to receive the MD5 digest
 * @param ctx the md5 context to finish
 */
void md5_final(uint8_t digest[16], md5_context_t *ctx)
{
	/* Number of bytes in ctx->in */
	int count = ctx->bytes[0] & 0x3f;
	uint8_t *p = (uint8_t *)ctx->in + count;

	/* Set the first char of padding to 0x80.  There is always room. */
	*p++ = 0x80;

	/* Bytes of padding needed to make 56 bytes (-8..55) */
	count = 56 - 1 - count;

	if (count < 0) {	/* Padding forces an extra block */
		memset(p, 0, count + 8);
		byte_swap(ctx->in, sizeof(ctx->in)/sizeof(ctx->in[0]));
		md5_transform(ctx->buf, ctx->in);
		p = (uint8_t *)ctx->in;
		count = 56;
	}
	memset(p, 0, count);
	byte_swap(ctx->in, sizeof(ctx->in)/sizeof(ctx->in[0])-2);

	/* Append length in bits and transform */
	ctx->in[14] = ctx->bytes[0] << 3;
	ctx->in[15] = (ctx->bytes[1] << 3) | (ctx->bytes[0] >> (32-3));
	md5_transform(ctx->buf, ctx->in);

	byte_swap(ctx->buf, sizeof(ctx->buf)/sizeof(ctx->buf[0]));
	memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

/**
 * @brief return the md5 digest string for an array of bytes
 *
 * @param buf buffer to hash
 * @param size number of bytes in the buffer
 * @result returns a pointer to a string containing the MD5 digest in hex
 */
const char *md5_digest(void *buff, size_t size)
{
	static char md5_str[16*2+1];
	md5_context_t ctx;
	uint8_t digest[16];
	uint8_t *src = (uint8_t *)buff;
	int i;

	md5_init(&ctx);
	md5_update(&ctx, src, size);
	md5_final(digest, &ctx);

	for (i = 0; i < 16; i++)
		sprintf(md5_str + 2 * i, "%02x", digest[i]);
	return md5_str;
}

/**
 * @brief test the MD5 digest with known results
 *
 * @result returns 0 on success, -1 on error
 */
int md5_test(void)
{
	const char *md5;

	md5 = md5_digest("", 0);
	if (strcmp(md5,"d41d8cd98f00b204e9800998ecf8427e"))
		return -1;
	md5 = md5_digest("a", 1);
	if (strcmp(md5,"0cc175b9c0f1b6a831c399e269772661"))
		return -1;
	md5 = md5_digest("abc", 3);
	if (strcmp(md5,"900150983cd24fb0d6963f7d28e17f72"))
		return -1;
	md5 = md5_digest("message digest", 14);
	if (strcmp(md5,"f96b697d7cb7938d525a2f31aaf161d0"))
		return -1;
	md5 = md5_digest("abcdefghijklmnopqrstuvwxyz", 26);
	if (strcmp(md5,"c3fcd3d76192e4007dfb496cca67e13b"))
		return -1;
	md5 = md5_digest("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
	if (strcmp(md5,"8215ef0796a20bcaaae116d3876c664a"))
		return -1;
	return 0;
}
