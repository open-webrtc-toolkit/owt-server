/**
 * @file sip/keepalive.c  SIP Keepalive
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_sys.h>
#include <re_uri.h>
#include <re_udp.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


static void destructor(void *arg)
{
	struct sip_keepalive *ka = arg;

	if (ka->kap)
		*ka->kap = NULL;

	list_unlink(&ka->le);
}


void sip_keepalive_signal(struct list *kal, int err)
{
	struct le *le = list_head(kal);

	while (le) {

		struct sip_keepalive *ka = le->data;
		sip_keepalive_h *kah = ka->kah;
		void *arg = ka->arg;

		le = le->next;

		list_unlink(&ka->le);
		mem_deref(ka);

		kah(err, arg);
	}
}


uint64_t sip_keepalive_wait(uint32_t interval)
{
	return interval * (800 + rand_u16() % 201);
}


/**
 * Start a keepalive handler on a SIP transport
 *
 * @param kap      Pointer to allocated keepalive object
 * @param sip      SIP Stack instance
 * @param msg      SIP Message
 * @param interval Keepalive interval in seconds
 * @param kah      Keepalive handler
 * @param arg      Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_keepalive_start(struct sip_keepalive **kap, struct sip *sip,
			const struct sip_msg *msg, uint32_t interval,
			sip_keepalive_h *kah, void *arg)
{
	struct sip_keepalive *ka;
	int err;

	if (!kap || !sip || !msg || !kah)
		return EINVAL;

	ka = mem_zalloc(sizeof(*ka), destructor);
	if (!ka)
		return ENOMEM;

	ka->kah = kah;
	ka->arg = arg;

	switch (msg->tp) {

	case SIP_TRANSP_UDP:
		err = sip_keepalive_udp(ka, sip, (struct udp_sock *)msg->sock,
					&msg->src, interval);
		break;

	case SIP_TRANSP_TCP:
	case SIP_TRANSP_TLS:
		err = sip_keepalive_tcp(ka, (struct sip_conn *)msg->sock,
					interval);
		break;

	default:
		err = EPROTONOSUPPORT;
		break;
	}

	if (err) {
		mem_deref(ka);
	}
	else {
		ka->kap = kap;
		*kap = ka;
	}

	return err;
}
