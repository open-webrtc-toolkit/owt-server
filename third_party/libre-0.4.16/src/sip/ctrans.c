/**
 * @file sip/ctrans.c  SIP Client Transaction
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
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


enum state {
	TRYING,
	CALLING,
	PROCEEDING,
	COMPLETED,
};


enum {
	COMPLETE_WAIT = 32000,
};


struct sip_ctrans {
	struct le he;
	struct sa dst;
	struct tmr tmr;
	struct tmr tmre;
	struct sip *sip;
	struct mbuf *mb;
	struct mbuf *mb_ack;
	struct sip_msg *req;
	struct sip_connqent *qent;
	char *met;
	char *branch;
	sip_resp_h *resph;
	void *arg;
	enum sip_transp tp;
	enum state state;
	uint32_t txc;
	bool invite;
};


static bool route_handler(const struct sip_hdr *hdr, const struct sip_msg *msg,
			  void *arg)
{
	(void)msg;
	return 0 != mbuf_printf(arg, "Route: %r\r\n", &hdr->val);
}


static int request_copy(struct mbuf **mbp, struct sip_ctrans *ct,
			const char *met, const struct sip_msg *resp)
{
	struct mbuf *mb;
	int err;

	if (!ct->req) {
		err = sip_msg_decode(&ct->req, ct->mb);
		if (err)
			return err;
	}

	mb = mbuf_alloc(1024);
	if (!mb)
		return ENOMEM;

	err  = mbuf_printf(mb, "%s %r SIP/2.0\r\n", met, &ct->req->ruri);
	err |= mbuf_printf(mb, "Via: %r\r\n", &ct->req->via.val);
	err |= mbuf_write_str(mb, "Max-Forwards: 70\r\n");
	err |= sip_msg_hdr_apply(ct->req, true, SIP_HDR_ROUTE,
				 route_handler, mb) ? ENOMEM : 0;
	err |= mbuf_printf(mb, "To: %r\r\n",
			   resp ? &resp->to.val : &ct->req->to.val);
	err |= mbuf_printf(mb, "From: %r\r\n", &ct->req->from.val);
	err |= mbuf_printf(mb, "Call-ID: %r\r\n", &ct->req->callid);
	err |= mbuf_printf(mb, "CSeq: %u %s\r\n", ct->req->cseq.num, met);
	if (ct->sip->software)
		err |= mbuf_printf(mb, "User-Agent: %s\r\n",ct->sip->software);
	err |= mbuf_write_str(mb, "Content-Length: 0\r\n\r\n");

	mb->pos = 0;

	if (err)
		mem_deref(mb);
	else
		*mbp = mb;

	return err;
}


static void destructor(void *arg)
{
	struct sip_ctrans *ct = arg;

	hash_unlink(&ct->he);
	tmr_cancel(&ct->tmr);
	tmr_cancel(&ct->tmre);
	mem_deref(ct->met);
	mem_deref(ct->branch);
	mem_deref(ct->qent);
	mem_deref(ct->req);
	mem_deref(ct->mb);
	mem_deref(ct->mb_ack);
}


static bool cmp_handler(struct le *le, void *arg)
{
	struct sip_ctrans *ct = le->data;
	const struct sip_msg *msg = arg;

	if (pl_strcmp(&msg->via.branch, ct->branch))
		return false;

	if (pl_strcmp(&msg->cseq.met, ct->met))
		return false;

	return true;
}


static void dummy_handler(int err, const struct sip_msg *msg, void *arg)
{
	(void)err;
	(void)msg;
	(void)arg;
}


static void terminate(struct sip_ctrans *ct, int err)
{
	switch (ct->state) {

	case TRYING:
	case CALLING:
	case PROCEEDING:
		ct->resph(err, NULL, ct->arg);
		break;

	default:
		break;
	}
}


static void transport_handler(int err, void *arg)
{
	struct sip_ctrans *ct = arg;

	terminate(ct, err);
	mem_deref(ct);
}


static void tmr_handler(void *arg)
{
	struct sip_ctrans *ct = arg;

	terminate(ct, ETIMEDOUT);
	mem_deref(ct);
}


static void retransmit_handler(void *arg)
{
	struct sip_ctrans *ct = arg;
	uint32_t timeout;
	int err;

	ct->txc++;

	switch (ct->state) {

	case TRYING:
		timeout = MIN(SIP_T1<<ct->txc, SIP_T2);
		break;

	case CALLING:
		timeout = SIP_T1<<ct->txc;
		break;

	case PROCEEDING:
		timeout = SIP_T2;
		break;

	default:
		return;
	}

	tmr_start(&ct->tmre, timeout, retransmit_handler, ct);

	err = sip_transp_send(&ct->qent, ct->sip, NULL, ct->tp, &ct->dst,
			      ct->mb, transport_handler, ct);
	if (err) {
		terminate(ct, err);
		mem_deref(ct);
	}
}


static void invite_response(struct sip_ctrans *ct, const struct sip_msg *msg)
{
	switch (ct->state) {

	case CALLING:
		tmr_cancel(&ct->tmr);
		tmr_cancel(&ct->tmre);
		/*@fallthrough@*/
	case PROCEEDING:
		if (msg->scode < 200) {
			ct->state = PROCEEDING;
			ct->resph(0, msg, ct->arg);
		}
		else if (msg->scode < 300) {
			ct->resph(0, msg, ct->arg);
			mem_deref(ct);
		}
		else {
			ct->state = COMPLETED;

			(void)request_copy(&ct->mb_ack, ct, "ACK", msg);
			(void)sip_send(ct->sip, NULL, ct->tp, &ct->dst,
				       ct->mb_ack);

			ct->resph(0, msg, ct->arg);

			if (sip_transp_reliable(ct->tp)) {
				mem_deref(ct);
				break;
			}

			tmr_start(&ct->tmr, COMPLETE_WAIT, tmr_handler, ct);
		}
		break;

	case COMPLETED:
		if (msg->scode < 300)
			break;

		(void)sip_send(ct->sip, NULL, ct->tp, &ct->dst, ct->mb_ack);
		break;

	default:
		break;
	}
}


static bool response_handler(const struct sip_msg *msg, void *arg)
{
	struct sip_ctrans *ct;
	struct sip *sip = arg;

	ct = list_ledata(hash_lookup(sip->ht_ctrans,
				     hash_joaat_pl(&msg->via.branch),
				     cmp_handler, (void *)msg));
	if (!ct)
		return false;

	if (ct->invite) {
		invite_response(ct, msg);
		return true;
	}

	switch (ct->state) {

	case TRYING:
	case PROCEEDING:
		if (msg->scode < 200) {
			ct->state = PROCEEDING;
			ct->resph(0, msg, ct->arg);
		}
		else {
			ct->state = COMPLETED;
			ct->resph(0, msg, ct->arg);

			if (sip_transp_reliable(ct->tp)) {
				mem_deref(ct);
				break;
			}

			tmr_start(&ct->tmr, SIP_T4, tmr_handler, ct);
			tmr_cancel(&ct->tmre);
		}
		break;

	default:
		break;
	}

	return true;
}


int sip_ctrans_request(struct sip_ctrans **ctp, struct sip *sip,
		       enum sip_transp tp, const struct sa *dst, char *met,
		       char *branch, struct mbuf *mb,
		       sip_resp_h *resph, void *arg)
{
	struct sip_ctrans *ct;
	int err;

	if (!sip || !dst || !met || !branch || !mb)
		return EINVAL;

	ct = mem_zalloc(sizeof(*ct), destructor);
	if (!ct)
		return ENOMEM;

	hash_append(sip->ht_ctrans, hash_joaat_str(branch), &ct->he, ct);

	ct->invite = !strcmp(met, "INVITE");
	ct->branch = mem_ref(branch);
	ct->met    = mem_ref(met);
	ct->mb     = mem_ref(mb);
	ct->dst    = *dst;
	ct->tp     = tp;
	ct->sip    = sip;
	ct->state  = ct->invite ? CALLING : TRYING;
	ct->resph  = resph ? resph : dummy_handler;
	ct->arg    = arg;

	err = sip_transp_send(&ct->qent, sip, NULL, tp, dst, mb,
			      transport_handler, ct);
	if (err)
		goto out;

	tmr_start(&ct->tmr, 64 * SIP_T1, tmr_handler, ct);

	if (!sip_transp_reliable(ct->tp))
		tmr_start(&ct->tmre, SIP_T1, retransmit_handler, ct);

 out:
	if (err)
		mem_deref(ct);
	else if (ctp)
		*ctp = ct;

	return err;
}


int sip_ctrans_cancel(struct sip_ctrans *ct)
{
	struct mbuf *mb = NULL;
	char *cancel = NULL;
	int err;

	if (!ct)
		return EINVAL;

	if (!ct->invite)
		return 0;

	switch (ct->state) {

	case PROCEEDING:
		tmr_start(&ct->tmr, 64 * SIP_T1, tmr_handler, ct);
		break;

	default:
		return EPROTO;
	}

	err = str_dup(&cancel, "CANCEL");
	if (err)
		goto out;

	err = request_copy(&mb, ct, cancel, NULL);
	if (err)
		goto out;

	err = sip_ctrans_request(NULL, ct->sip, ct->tp, &ct->dst, cancel,
				 ct->branch, mb, NULL, NULL);
	if (err)
		goto out;

 out:
	mem_deref(cancel);
	mem_deref(mb);

	return err;
}


int sip_ctrans_init(struct sip *sip, uint32_t sz)
{
	int err;

	err = sip_listen(NULL, sip, false, response_handler, sip);
	if (err)
		return err;

	return hash_alloc(&sip->ht_ctrans, sz);
}


static const char *statename(enum state state)
{
	switch (state) {

	case TRYING:     return "TRYING";
	case CALLING:    return "CALLING";
	case PROCEEDING: return "PROCEEDING";
	case COMPLETED:  return "COMPLETED";
	default:         return "???";
	}
}


static bool debug_handler(struct le *le, void *arg)
{
	struct sip_ctrans *ct = le->data;
	struct re_printf *pf = arg;

	(void)re_hprintf(pf, "  %-10s %-10s %2llus (%s)\n",
			 ct->met,
			 statename(ct->state),
			 tmr_get_expire(&ct->tmr)/1000,
			 ct->branch);

	return false;
}


int sip_ctrans_debug(struct re_printf *pf, const struct sip *sip)
{
	int err;

	err = re_hprintf(pf, "client transactions:\n");
	hash_apply(sip->ht_ctrans, debug_handler, pf);

	return err;
}
