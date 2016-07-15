/**
 * @file via.c  SIP Via decode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_uri.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_msg.h>
#include <re_sip.h>


static int decode_hostport(const struct pl *hostport, struct pl *host,
			   struct pl *port)
{
	/* Try IPv6 first */
	if (!re_regex(hostport->p, hostport->l, "\\[[0-9a-f:]+\\][:]*[0-9]*",
		      host, NULL, port))
		return 0;

	/* Then non-IPv6 host */
	return re_regex(hostport->p, hostport->l, "[^:]+[:]*[0-9]*",
			host, NULL, port);
}


/**
 * Decode a pointer-length string into a SIP Via header
 *
 * @param via SIP Via header
 * @param pl  Pointer-length string
 *
 * @return 0 for success, otherwise errorcode
 */
int sip_via_decode(struct sip_via *via, const struct pl *pl)
{
	struct pl transp, host, port;
	int err;

	if (!via || !pl)
		return EINVAL;

	err = re_regex(pl->p, pl->l,
		       "SIP[  \t\r\n]*/[ \t\r\n]*2.0[ \t\r\n]*/[ \t\r\n]*"
		       "[A-Z]+[ \t\r\n]*[^; \t\r\n]+[ \t\r\n]*[^]*",
		       NULL, NULL, NULL, NULL, &transp,
		       NULL, &via->sentby, NULL, &via->params);
	if (err)
		return err;

	if (!pl_strcmp(&transp, "TCP"))
		via->tp = SIP_TRANSP_TCP;
	else if (!pl_strcmp(&transp, "TLS"))
		via->tp = SIP_TRANSP_TLS;
	else if (!pl_strcmp(&transp, "UDP"))
		via->tp = SIP_TRANSP_UDP;
	else
		via->tp = SIP_TRANSP_NONE;

	err = decode_hostport(&via->sentby, &host, &port);
	if (err)
		return err;

	sa_init(&via->addr, AF_INET);

	(void)sa_set(&via->addr, &host, 0);

	if (pl_isset(&port))
		sa_set_port(&via->addr, pl_u32(&port));

	via->val = *pl;

	return msg_param_decode(&via->params, "branch", &via->branch);
}
