/**
 * @file endian.c  Endianness converting routines
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_sys.h>


/*
 * These routes are working on both little-endian and big-endian platforms.
 */


/**
 * Convert a 16-bit value from host order to little endian
 *
 * @param v 16-bit in host order
 *
 * @return 16-bit little endian value
 */
uint16_t sys_htols(uint16_t v)
{
	uint8_t *p = (uint8_t *)&v;
	uint16_t l = 0;

	l |= (uint16_t)*p++ << 0;
	l |= (uint16_t)*p   << 8;

	return l;
}


/**
 * Convert a 32-bit value from host order to little endian
 *
 * @param v 32-bit in host order
 *
 * @return 32-bit little endian value
 */
uint32_t sys_htoll(uint32_t v)
{
	uint8_t *p = (uint8_t *)&v;
	uint32_t l = 0;

	l |= (uint32_t)*p++ << 0;
	l |= (uint32_t)*p++ << 8;
	l |= (uint32_t)*p++ << 16;
	l |= (uint32_t)*p   << 24;

	return l;
}


/**
 * Convert a 16-bit value from little endian to host order
 *
 * @param v 16-bit little endian value
 *
 * @return 16-bit value in host order
 */
uint16_t sys_ltohs(uint16_t v)
{
	uint16_t s;
	uint8_t *p = (uint8_t *)&s;

	*p++ = v>>0 & 0xff;
	*p   = v>>8 & 0xff;

	return s;
}


/**
 * Convert a 32-bit value from little endian to host order
 *
 * @param v 32-bit little endian value
 *
 * @return 32-bit value in host order
 */
uint32_t sys_ltohl(uint32_t v)
{
	uint32_t h;
	uint8_t *p = (uint8_t *)&h;

	*p++ = v>>0  & 0xff;
	*p++ = v>>8  & 0xff;
	*p++ = v>>16 & 0xff;
	*p   = v>>24 & 0xff;

	return h;
}


/**
 * Convert a 64-bit value from host to network byte-order
 *
 * @param v 64-bit host byte-order value
 *
 * @return 64-bit value in network byte-order
 */
uint64_t sys_htonll(uint64_t v)
{
	uint64_t h = 0;
	uint8_t *p = (uint8_t *)&v;

	h |= (uint64_t)*p++ << 56;
	h |= (uint64_t)*p++ << 48;
	h |= (uint64_t)*p++ << 40;
	h |= (uint64_t)*p++ << 32;
	h |= (uint64_t)*p++ << 24;
	h |= (uint64_t)*p++ << 16;
	h |= (uint64_t)*p++ << 8;
	h |= (uint64_t)*p   << 0;

	return h;
}


/**
 * Convert a 64-bit value from network to host byte-order
 *
 * @param v 64-bit network byte-order value
 *
 * @return 64-bit value in host byte-order
 */
uint64_t sys_ntohll(uint64_t v)
{
	uint64_t h;
	uint8_t *p = (uint8_t *)&h;

	*p++ = (uint8_t) (v>>56 & 0xff);
	*p++ = (uint8_t) (v>>48 & 0xff);
	*p++ = (uint8_t) (v>>40 & 0xff);
	*p++ = (uint8_t) (v>>32 & 0xff);
	*p++ = (uint8_t) (v>>24 & 0xff);
	*p++ = (uint8_t) (v>>16 & 0xff);
	*p++ = (uint8_t) (v>>8  & 0xff);
	*p   = (uint8_t) (v>>0  & 0xff);

	return h;
}
