/**
 * @file stun/req.c  STUN request
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
 * Send a STUN request using a client transaction
 *
 * @param ctp     Pointer to allocated client transaction (optional)
 * @param stun    STUN Instance
 * @param proto   Transport Protocol
 * @param sock    Socket; UDP (struct udp_sock) or TCP (struct tcp_conn)
 * @param dst     Destination network address
 * @param presz   Number of bytes in preamble, if sending over TURN
 * @param method  STUN Method
 * @param key     Authentication key (optional)
 * @param keylen  Number of bytes in authentication key
 * @param fp      Use STUN Fingerprint attribute
 * @param resph   Response handler
 * @param arg     Response handler argument
 * @param attrc   Number of attributes to encode (variable arguments)
 * @param ...     Variable list of attribute-tuples
 *                Each attribute has 2 arguments, attribute type and value
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_request(struct stun_ctrans **ctp, struct stun *stun, int proto,
		 void *sock, const struct sa *dst, size_t presz,
		 uint16_t method, const uint8_t *key, size_t keylen, bool fp,
		 stun_resp_h *resph, void *arg, uint32_t attrc, ...)
{
	uint8_t tid[STUN_TID_SIZE];
	struct mbuf *mb;
	uint32_t i;
	va_list ap;
	int err;

	if (!stun)
		return EINVAL;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	for (i=0; i<STUN_TID_SIZE; i++)
		tid[i] = rand_u32();

	va_start(ap, attrc);
	mb->pos = presz;
	err = stun_msg_vencode(mb, method, STUN_CLASS_REQUEST,
			       tid, NULL, key, keylen, fp, 0x00, attrc, ap);
	va_end(ap);
	if (err)
		goto out;

	mb->pos = presz;
	err = stun_ctrans_request(ctp, stun, proto, sock, dst, mb, tid, method,
				  key, keylen, resph, arg);
	if (err)
		goto out;

 out:
	mem_deref(mb);

	return err;
}
