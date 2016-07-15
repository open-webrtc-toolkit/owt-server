/**
 * @file rand.c  Random generator
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <stdlib.h>
#ifdef USE_OPENSSL
#include <openssl/rand.h>
#include <openssl/err.h>
#endif
#include <re_types.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sys.h>


#define DEBUG_MODULE "rand"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#ifndef RELEASE
#define RAND_DEBUG 1  /**< Enable random debugging */
#endif


#if RAND_DEBUG
static bool inited = false;
/** Check random state */
#define RAND_CHECK							\
	if (!inited) {							\
		DEBUG_WARNING("%s: random not inited\n", __REFUNC__);	\
	}
#else
#define RAND_CHECK if (0) {}
#endif


/**
 * Initialise random number generator
 */
void rand_init(void)
{
#ifndef USE_OPENSSL
	srand((uint32_t) tmr_jiffies());
#endif

#if RAND_DEBUG
	inited = true;
#endif
}


/**
 * Generate an unsigned 16-bit random value
 *
 * @return 16-bit random value
 */
uint16_t rand_u16(void)
{
	RAND_CHECK;

	/* Use higher-order bits (see man 3 rand) */
	return rand_u32() >> 16;
}


/**
 * Generate an unsigned 32-bit random value
 *
 * @return 32-bit random value
 */
uint32_t rand_u32(void)
{
	uint32_t v;

	RAND_CHECK;

#ifdef USE_OPENSSL
	v = 0;
	if (RAND_bytes((unsigned char *)&v, sizeof(v)) <= 0) {
		DEBUG_WARNING("RAND_bytes() error: %i\n",
			      ERR_GET_REASON(ERR_get_error()));
		ERR_clear_error();
	}
#elif defined(HAVE_ARC4RANDOM)
	v = arc4random();
#elif defined(WIN32)
	v = (rand() << 16) + rand(); /* note: 16-bit rand */
#else
	v = rand();
#endif

	return v;
}


/**
 * Generate an unsigned 64-bit random value
 *
 * @return 64-bit random value
 */
uint64_t rand_u64(void)
{
	RAND_CHECK;

	return (uint64_t)rand_u32()<<32 | rand_u32();
}


/**
 * Generate a random printable character
 *
 * @return Random printable character
 */
char rand_char(void)
{
	char c;

	RAND_CHECK;

	do {
		c = 0x30 + (rand_u16() % 0x4f);
	} while (!isalpha(c) && !isdigit(c));

	return c;
}


/**
 * Generate a string of random characters
 *
 * @param str  Pointer to string
 * @param size Size of string
 */
void rand_str(char *str, size_t size)
{
	if (!str || !size)
		return;

	RAND_CHECK;

	str[--size] = '\0';
	while (size--) {
		str[size] = rand_char();
	}
}


/**
 * Generate a set of random bytes
 *
 * @param p    Pointer to buffer
 * @param size Size of buffer
 */
void rand_bytes(uint8_t *p, size_t size)
{
#ifdef USE_OPENSSL
	if (RAND_bytes(p, (int)size) <= 0) {
		DEBUG_WARNING("RAND_bytes() error: %i\n",
			      ERR_GET_REASON(ERR_get_error()));
		ERR_clear_error();
	}
#elif defined (HAVE_ARC4RANDOM)
	arc4random_buf(p, size);
#else
	while (size--) {
		p[size] = rand_u32();
	}
#endif
}
