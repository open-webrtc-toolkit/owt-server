/**
 * @file stun/rep.c  STUN reply
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include "stun.h"


/**
 * Send a STUN response message
 *
 * @param proto   Transport Protocol
 * @param sock    Socket; UDP (struct udp_sock) or TCP (struct tcp_conn)
 * @param dst     Destination network address
 * @param presz   Number of bytes in preamble, if sending over TURN
 * @param req     Matching STUN request
 * @param key     Authentication key (optional)
 * @param keylen  Number of bytes in authentication key
 * @param fp      Use STUN Fingerprint attribute
 * @param attrc   Number of attributes to encode (variable arguments)
 * @param ...     Variable list of attribute-tuples
 *                Each attribute has 2 arguments, attribute type and value
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_reply(int proto, void *sock, const struct sa *dst, size_t presz,
	       const struct stun_msg *req, const uint8_t *key,
	       size_t keylen, bool fp, uint32_t attrc, ...)
{
	struct mbuf *mb = NULL;
	int err = ENOMEM;
	va_list ap;

	if (!sock || !req)
		return EINVAL;

	mb = mbuf_alloc(256);
	if (!mb)
		goto out;

	va_start(ap, attrc);
	mb->pos = presz;
	err = stun_msg_vencode(mb, stun_msg_method(req),
			       STUN_CLASS_SUCCESS_RESP, stun_msg_tid(req),
			       NULL, key, keylen, fp, 0x00, attrc, ap);
	va_end(ap);
	if (err)
		goto out;

	mb->pos = presz;
	err = stun_send(proto, sock, dst, mb);

 out:
	mem_deref(mb);

	return err;
}


/**
 * Send a STUN error response
 *
 * @param proto   Transport Protocol
 * @param sock    Socket; UDP (struct udp_sock) or TCP (struct tcp_conn)
 * @param dst     Destination network address
 * @param presz   Number of bytes in preamble, if sending over TURN
 * @param req     Matching STUN request
 * @param scode   Status code
 * @param reason  Reason string
 * @param key     Authentication key (optional)
 * @param keylen  Number of bytes in authentication key
 * @param fp      Use STUN Fingerprint attribute
 * @param attrc   Number of attributes to encode (variable arguments)
 * @param ...     Variable list of attribute-tuples
 *                Each attribute has 2 arguments, attribute type and value
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_ereply(int proto, void *sock, const struct sa *dst, size_t presz,
		const struct stun_msg *req, uint16_t scode,
		const char *reason, const uint8_t *key, size_t keylen,
		bool fp, uint32_t attrc, ...)
{
	struct stun_errcode ec;
	struct mbuf *mb = NULL;
	int err = ENOMEM;
	va_list ap;

	if (!sock || !req || !scode || !reason)
		return EINVAL;

	mb = mbuf_alloc(256);
	if (!mb)
		goto out;

	ec.code = scode;
	ec.reason = (char *)reason;

	va_start(ap, attrc);
	mb->pos = presz;
	err = stun_msg_vencode(mb, stun_msg_method(req), STUN_CLASS_ERROR_RESP,
			       stun_msg_tid(req), &ec, key, keylen,
			       fp, 0x00, attrc, ap);
	va_end(ap);
	if (err)
		goto out;

	mb->pos = presz;
	err = stun_send(proto, sock, dst, mb);

 out:
	mem_deref(mb);

	return err;
}
