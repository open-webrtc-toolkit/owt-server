/**
 * @file keepalive_udp.c  SIP UDP Keepalive
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_fmt.h>
#include <re_uri.h>
#include <re_sys.h>
#include <re_tmr.h>
#include <re_udp.h>
#include <re_stun.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


enum {
	UDP_KEEPALIVE_INTVAL = 29,
};


struct sip_udpconn {
	struct le he;
	struct list kal;
	struct tmr tmr_ka;
	struct sa maddr;
	struct sa paddr;
	struct udp_sock *us;
	struct stun_ctrans *ct;
	struct stun *stun;
	uint32_t ka_interval;
};


static void udpconn_keepalive_handler(void *arg);


static void destructor(void *arg)
{
	struct sip_udpconn *uc = arg;

	list_flush(&uc->kal);
	hash_unlink(&uc->he);
	tmr_cancel(&uc->tmr_ka);
	mem_deref(uc->ct);
	mem_deref(uc->us);
	mem_deref(uc->stun);
}


static void udpconn_close(struct sip_udpconn *uc, int err)
{
	sip_keepalive_signal(&uc->kal, err);
	hash_unlink(&uc->he);
	tmr_cancel(&uc->tmr_ka);
	uc->ct = mem_deref(uc->ct);
	uc->us = mem_deref(uc->us);
	uc->stun = mem_deref(uc->stun);
}


static void stun_response_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct sip_udpconn *uc = arg;
	struct stun_attr *attr;
	(void)reason;

	if (err || scode) {
		err = err ? err : EPROTO;
		goto out;
	}

	attr = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
	if (!attr) {
		attr = stun_msg_attr(msg, STUN_ATTR_MAPPED_ADDR);
		if (!attr) {
			err = EPROTO;
			goto out;
		}
	}

	if (!sa_isset(&uc->maddr, SA_ALL)) {
		uc->maddr = attr->v.sa;
	}
	else if (!sa_cmp(&uc->maddr, &attr->v.sa, SA_ALL)) {
		err = ENOTCONN;
		goto out;
	}

 out:
	if (err) {
		udpconn_close(uc, err);
		mem_deref(uc);
	}
	else {
		tmr_start(&uc->tmr_ka, sip_keepalive_wait(uc->ka_interval),
			  udpconn_keepalive_handler, uc);
	}
}


static void udpconn_keepalive_handler(void *arg)
{
	struct sip_udpconn *uc = arg;
	int err;

	if (!uc->kal.head) {
		/* no need for us anymore */
		udpconn_close(uc, 0);
		mem_deref(uc);
		return;
	}

	err = stun_request(&uc->ct, uc->stun, IPPROTO_UDP, uc->us,
			   &uc->paddr, 0, STUN_METHOD_BINDING, NULL, 0,
			   false, stun_response_handler, uc, 1,
			   STUN_ATTR_SOFTWARE, stun_software);
	if (err) {
		udpconn_close(uc, err);
		mem_deref(uc);
	}
}


static struct sip_udpconn *udpconn_find(struct sip *sip, struct udp_sock *us,
					const struct sa *paddr)
{
	struct le *le;

	le = list_head(hash_list(sip->ht_udpconn, sa_hash(paddr, SA_ALL)));

	for (; le; le = le->next) {

		struct sip_udpconn *uc = le->data;

		if (!sa_cmp(&uc->paddr, paddr, SA_ALL))
			continue;

		if (uc->us != us)
			continue;

		return uc;
	}

	return NULL;
}


int  sip_keepalive_udp(struct sip_keepalive *ka, struct sip *sip,
		       struct udp_sock *us, const struct sa *paddr,
		       uint32_t interval)
{
	struct sip_udpconn *uc;

	if (!ka || !sip || !us || !paddr)
		return EINVAL;

	uc = udpconn_find(sip, us, paddr);
	if (!uc) {
		uc = mem_zalloc(sizeof(*uc), destructor);
		if (!uc)
			return ENOMEM;

		hash_append(sip->ht_udpconn, sa_hash(paddr, SA_ALL),
			    &uc->he, uc);

		uc->paddr = *paddr;
		uc->stun  = mem_ref(sip->stun);
		uc->us    = mem_ref(us);
		uc->ka_interval = interval ? interval : UDP_KEEPALIVE_INTVAL;

		/* learn mapped address immediately */
		tmr_start(&uc->tmr_ka, 0, udpconn_keepalive_handler, uc);
	}

	list_append(&uc->kal, &ka->le, ka);

	return 0;
}
