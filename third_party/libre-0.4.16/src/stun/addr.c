/**
 * @file stun/addr.c  STUN Address encoding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include "stun.h"


static void in6_xor_tid(uint8_t *in6, const uint8_t *tid)
{
	int i;

	/* XOR with Magic Cookie (alignment safe) */
	in6[0] ^= 0x21;
	in6[1] ^= 0x12;
	in6[2] ^= 0xa4;
	in6[3] ^= 0x42;

	for (i=0; i<STUN_TID_SIZE; i++)
		in6[4+i] ^= tid[i];
}


int stun_addr_encode(struct mbuf *mb, const struct sa *addr,
		     const uint8_t *tid)
{
#ifdef HAVE_INET6
	uint8_t addr6[16];
#endif
	uint16_t port;
	uint32_t addr4;
	int err = 0;

	if (!mb || !addr)
		return EINVAL;

	port = tid ? sa_port(addr)^(STUN_MAGIC_COOKIE>>16) : sa_port(addr);

	switch (sa_af(addr)) {

	case AF_INET:
		addr4 = tid ? sa_in(addr)^STUN_MAGIC_COOKIE : sa_in(addr);

		err |= mbuf_write_u8(mb, 0);
		err |= mbuf_write_u8(mb, STUN_AF_IPv4);
		err |= mbuf_write_u16(mb, htons(port));
		err |= mbuf_write_u32(mb, htonl(addr4));
		break;

#ifdef HAVE_INET6
	case AF_INET6:
		sa_in6(addr, addr6);
		if (tid)
			in6_xor_tid(addr6, tid);

		err |= mbuf_write_u8(mb, 0);
		err |= mbuf_write_u8(mb, STUN_AF_IPv6);
		err |= mbuf_write_u16(mb, htons(port));
		err |= mbuf_write_mem(mb, addr6, 16);
		break;
#endif
	default:
		err = EAFNOSUPPORT;
		break;
	}

	return err;
}


int stun_addr_decode(struct mbuf *mb, struct sa *addr, const uint8_t *tid)
{
	uint8_t family, addr6[16];
	uint32_t addr4;
	uint16_t port;

	if (!mb || !addr)
		return EINVAL;

	if (mbuf_get_left(mb) < 4)
		return EBADMSG;

	(void)mbuf_read_u8(mb);
	family = mbuf_read_u8(mb);
	port = ntohs(mbuf_read_u16(mb));

	if (tid)
		port ^= STUN_MAGIC_COOKIE>>16;

	switch (family) {

	case STUN_AF_IPv4:
		if (mbuf_get_left(mb) < 4)
			return EBADMSG;

		addr4 = ntohl(mbuf_read_u32(mb));
		if (tid)
			addr4 ^= STUN_MAGIC_COOKIE;

		sa_set_in(addr, addr4, port);
		break;

	case STUN_AF_IPv6:
		if (mbuf_get_left(mb) < 16)
			return EBADMSG;

		(void)mbuf_read_mem(mb, addr6, 16);
		if (tid)
			in6_xor_tid(addr6, tid);

		sa_set_in6(addr, addr6, port);
		break;

	default:
		return EAFNOSUPPORT;
	}

	return 0;
}
