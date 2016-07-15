/**
 * @file sip/addr.c  SIP Address decode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_uri.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_msg.h>
#include <re_sip.h>


/**
 * Decode a pointer-length string into a SIP Address object
 *
 * @param addr SIP Address object
 * @param pl   Pointer-length string
 *
 * @return 0 for success, otherwise errorcode
 */
int sip_addr_decode(struct sip_addr *addr, const struct pl *pl)
{
	int err;

	if (!addr || !pl)
		return EINVAL;

	memset(addr, 0, sizeof(*addr));

	if (0 == re_regex(pl->p, pl->l, "[~ \t\r\n<]*[ \t\r\n]*<[^>]+>[^]*",
			  &addr->dname, NULL, &addr->auri, &addr->params)) {

		if (!addr->dname.l)
			addr->dname.p = NULL;

		if (!addr->params.l)
			addr->params.p = NULL;
	}
	else {
		memset(addr, 0, sizeof(*addr));

		if (re_regex(pl->p, pl->l, "[^;]+[^]*",
			     &addr->auri, &addr->params))
			return EBADMSG;
	}

	err = uri_decode(&addr->uri, &addr->auri);
	if (err)
		memset(addr, 0, sizeof(*addr));

	return err;
}
