/**
 * @file sip/reply.c  SIP Reply
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_uri.h>
#include <re_udp.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


static int vreplyf(struct sip_strans **stp, struct mbuf **mbp, bool trans,
		   struct sip *sip, const struct sip_msg *msg, bool rec_route,
		   uint16_t scode, const char *reason,
		   const char *fmt, va_list ap)
{
	bool rport = false;
	uint32_t viac = 0;
	struct mbuf *mb;
	struct sa dst;
	struct le *le;
	int err;

	if (!sip || !msg || !reason)
		return EINVAL;

	if (!pl_strcmp(&msg->met, "ACK"))
		return 0;

	mb = mbuf_alloc(1024);
	if (!mb) {
		err = ENOMEM;
		goto out;
	}

	err = mbuf_printf(mb, "SIP/2.0 %u %s\r\n", scode, reason);

	for (le = msg->hdrl.head; le; le = le->next) {

		struct sip_hdr *hdr = le->data;
		struct pl rp;

		switch (hdr->id) {

		case SIP_HDR_VIA:
			err |= mbuf_printf(mb, "%r: ", &hdr->name);
			if (viac++) {
				err |= mbuf_printf(mb, "%r\r\n", &hdr->val);
				break;
			}

			if (!msg_param_exists(&msg->via.params, "rport", &rp)){
				err |= mbuf_write_pl_skip(mb, &hdr->val, &rp);
				err |= mbuf_printf(mb, ";rport=%u",
						   sa_port(&msg->src));
				rport = true;
			}
			else
				err |= mbuf_write_pl(mb, &hdr->val);

			if (rport || !sa_cmp(&msg->src, &msg->via.addr,
					     SA_ADDR))
				err |= mbuf_printf(mb, ";received=%j",
						   &msg->src);

			err |= mbuf_write_str(mb, "\r\n");
			break;

		case SIP_HDR_TO:
			err |= mbuf_printf(mb, "%r: %r", &hdr->name,
					   &hdr->val);
			if (!pl_isset(&msg->to.tag) && scode > 100)
				err |= mbuf_printf(mb, ";tag=%016llx",
						   msg->tag);
			err |= mbuf_write_str(mb, "\r\n");
			break;

		case SIP_HDR_RECORD_ROUTE:
			if (!rec_route)
				break;

			/*@fallthrough@*/

		case SIP_HDR_FROM:
		case SIP_HDR_CALL_ID:
		case SIP_HDR_CSEQ:
			err |= mbuf_printf(mb, "%r: %r\r\n",
					   &hdr->name, &hdr->val);
			break;

		default:
			break;
		}
	}

	if (sip->software)
		err |= mbuf_printf(mb, "Server: %s\r\n", sip->software);

	if (fmt)
		err |= mbuf_vprintf(mb, fmt, ap);
	else
		err |= mbuf_printf(mb, "Content-Length: 0\r\n\r\n");

	if (err)
		goto out;

	mb->pos = 0;

	sip_reply_addr(&dst, msg, rport);

	if (trans) {
		err = sip_strans_reply(stp, sip, msg, &dst, scode, mb);
	}
	else {
		err = sip_send(sip, msg->sock, msg->tp, &dst, mb);
	}

 out:
	if (err && stp)
		*stp = mem_deref(*stp);

	if (!err && mbp)
		*mbp = mb;
	else
		mem_deref(mb);

	return err;
}


/**
 * Formatted reply using Server Transaction
 *
 * @param stp       Pointer to allocated SIP Server Transaction (optional)
 * @param mbp       Pointer to allocated SIP message buffer (optional)
 * @param sip       SIP Stack instance
 * @param msg       Incoming SIP message
 * @param rec_route True to copy Record-Route headers
 * @param scode     Response status code
 * @param reason    Response reason phrase
 * @param fmt       Additional formatted SIP headers and body, otherwise NULL
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_treplyf(struct sip_strans **stp, struct mbuf **mbp, struct sip *sip,
		const struct sip_msg *msg, bool rec_route, uint16_t scode,
		const char *reason, const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = vreplyf(stp, mbp, true, sip, msg, rec_route, scode, reason,
		      fmt, ap);
	va_end(ap);

	return err;
}


/**
 * Reply using Server Transaction
 *
 * @param stp    Pointer to allocated SIP Server Transaction (optional)
 * @param sip    SIP Stack instance
 * @param msg    Incoming SIP message
 * @param scode  Response status code
 * @param reason Response reason phrase
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_treply(struct sip_strans **stp, struct sip *sip,
	       const struct sip_msg *msg, uint16_t scode, const char *reason)
{
	return sip_treplyf(stp, NULL, sip, msg, false, scode, reason, NULL);
}


/**
 * Stateless formatted reply
 *
 * @param sip    SIP Stack instance
 * @param msg    Incoming SIP message
 * @param scode  Response status code
 * @param reason Response reason phrase
 * @param fmt    Additional formatted SIP headers and body, otherwise NULL
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_replyf(struct sip *sip, const struct sip_msg *msg, uint16_t scode,
	       const char *reason, const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = vreplyf(NULL, NULL, false, sip, msg, false, scode, reason,
		      fmt, ap);
	va_end(ap);

	return err;
}


/**
 * Stateless reply
 *
 * @param sip    SIP Stack instance
 * @param msg    Incoming SIP message
 * @param scode  Response status code
 * @param reason Response reason phrase
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_reply(struct sip *sip, const struct sip_msg *msg, uint16_t scode,
	      const char *reason)
{
	return sip_replyf(sip, msg, scode, reason, NULL);
}


/**
 * Get the reply address from a SIP message
 *
 * @param addr  Network address, set on return
 * @param msg   SIP message
 * @param rport Rport value
 */
void sip_reply_addr(struct sa *addr, const struct sip_msg *msg, bool rport)
{
	uint16_t port;
	struct pl pl;

	if (!addr || !msg)
		return;

	port = sa_port(&msg->via.addr);
	*addr = msg->src;

	switch (msg->tp) {

	case SIP_TRANSP_UDP:
		if (!msg_param_decode(&msg->via.params, "maddr", &pl)) {
			(void)sa_set(addr, &pl,sip_transp_port(msg->tp, port));
			break;
		}

		if (rport)
			break;

		/*@fallthrough@*/

	case SIP_TRANSP_TCP:
	case SIP_TRANSP_TLS:
		sa_set_port(addr, sip_transp_port(msg->tp, port));
		break;

	default:
		break;
	}
}
