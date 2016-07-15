/**
 * @file ack.c  SIP Session ACK
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_fmt.h>
#include <re_uri.h>
#include <re_tmr.h>
#include <re_msg.h>
#include <re_sip.h>
#include <re_sipsess.h>
#include "sipsess.h"


struct sipsess_ack {
	struct le he;
	struct tmr tmr;
	struct sa dst;
	struct sip_request *req;
	struct sip_dialog *dlg;
	struct mbuf *mb;
	enum sip_transp tp;
	uint32_t cseq;
};


static void destructor(void *arg)
{
	struct sipsess_ack *ack = arg;

	hash_unlink(&ack->he);
	tmr_cancel(&ack->tmr);
	mem_deref(ack->req);
	mem_deref(ack->dlg);
	mem_deref(ack->mb);
}


static void tmr_handler(void *arg)
{
	struct sipsess_ack *ack = arg;

	mem_deref(ack);
}


static int send_handler(enum sip_transp tp, const struct sa *src,
			const struct sa *dst, struct mbuf *mb, void *arg)
{
	struct sipsess_ack *ack = arg;
	(void)src;

	mem_deref(ack->mb);
	ack->mb = mem_ref(mb);
	ack->dst = *dst;
	ack->tp  = tp;

	tmr_start(&ack->tmr, 64 * SIP_T1, tmr_handler, ack);

	return 0;
}


static void resp_handler(int err, const struct sip_msg *msg, void *arg)
{
	struct sipsess_ack *ack = arg;
	(void)err;
	(void)msg;

	mem_deref(ack);
}


int sipsess_ack(struct sipsess_sock *sock, struct sip_dialog *dlg,
		uint32_t cseq, struct sip_auth *auth,
		const char *ctype, struct mbuf *desc)
{
	struct sipsess_ack *ack;
	int err;

	ack = mem_zalloc(sizeof(*ack), destructor);
	if (!ack)
		return ENOMEM;

	hash_append(sock->ht_ack,
		    hash_joaat_str(sip_dialog_callid(dlg)),
		    &ack->he, ack);

	ack->dlg  = mem_ref(dlg);
	ack->cseq = cseq;

	err = sip_drequestf(&ack->req, sock->sip, false, "ACK", dlg, cseq,
			    auth, send_handler, resp_handler, ack,
			    "%s%s%s"
			    "Content-Length: %zu\r\n"
			    "\r\n"
			    "%b",
			    desc ? "Content-Type: " : "",
			    desc ? ctype : "",
			    desc ? "\r\n" : "",
			    desc ? mbuf_get_left(desc) : (size_t)0,
			    desc ? mbuf_buf(desc) : NULL,
			    desc ? mbuf_get_left(desc) : (size_t)0);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(ack);

	return err;
}


static bool cmp_handler(struct le *le, void *arg)
{
	struct sipsess_ack *ack = le->data;
	const struct sip_msg *msg = arg;

	if (!sip_dialog_cmp(ack->dlg, msg))
		return false;

	if (ack->cseq != msg->cseq.num)
		return false;

	return true;
}


int sipsess_ack_again(struct sipsess_sock *sock, const struct sip_msg *msg)
{
	struct sipsess_ack *ack;

	ack = list_ledata(hash_lookup(sock->ht_ack,
				      hash_joaat_pl(&msg->callid),
				      cmp_handler, (void *)msg));
	if (!ack)
		return ENOENT;

	return sip_send(sock->sip, NULL, ack->tp, &ack->dst, ack->mb);
}
