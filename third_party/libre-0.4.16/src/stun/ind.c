/**
 * @file ind.c  STUN Indication
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_sys.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include "stun.h"


/**
 * Send a STUN Indication message
 *
 * @param proto   Transport Protocol
 * @param sock    Socket; UDP (struct udp_sock) or TCP (struct tcp_conn)
 * @param dst     Destination network address
 * @param presz   Number of bytes in preamble, if sending over TURN
 * @param method  STUN Method
 * @param key     Authentication key (optional)
 * @param keylen  Number of bytes in authentication key
 * @param fp      Use STUN Fingerprint attribute
 * @param attrc   Number of attributes to encode (variable arguments)
 * @param ...     Variable list of attribute-tuples
 *                Each attribute has 2 arguments, attribute type and value
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_indication(int proto, void *sock, const struct sa *dst, size_t presz,
		    uint16_t method, const uint8_t *key, size_t keylen,
		    bool fp, uint32_t attrc, ...)
{
	uint8_t tid[STUN_TID_SIZE];
	struct mbuf *mb;
	va_list ap;
	uint32_t i;
	int err;

	if (!sock)
		return EINVAL;

	mb = mbuf_alloc(2048);
	if (!mb)
		return ENOMEM;

	for (i=0; i<STUN_TID_SIZE; i++)
		tid[i] = rand_u32();

	va_start(ap, attrc);
	mb->pos = presz;
	err = stun_msg_vencode(mb, method, STUN_CLASS_INDICATION, tid, NULL,
			       key, keylen, fp, 0x00, attrc, ap);
	va_end(ap);
	if (err)
		goto out;

	mb->pos = presz;
	err = stun_send(proto, sock, dst, mb);

 out:
	mem_deref(mb);

	return err;
}
