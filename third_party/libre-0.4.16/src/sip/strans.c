/**
 * @file strans.c  SIP Server Transaction
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
#include <re_sys.h>
#include <re_tmr.h>
#include <re_udp.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


enum state {
	TRYING,
	PROCEEDING,
	ACCEPTED,
	COMPLETED,
	CONFIRMED,
};


struct sip_strans {
	struct le he;
	struct le he_mrg;
	struct tmr tmr;
	struct tmr tmrg;
	struct sa dst;
	struct sip *sip;
	struct sip_msg *msg;
	struct mbuf *mb;
	sip_cancel_h *cancelh;
	void *arg;
	enum state state;
	uint32_t txc;
	bool invite;
};


static void destructor(void *arg)
{
	struct sip_strans *st = arg;

	hash_unlink(&st->he);
	hash_unlink(&st->he_mrg);
	tmr_cancel(&st->tmr);
	tmr_cancel(&st->tmrg);
	mem_deref(st->msg);
	mem_deref(st->mb);
}


static bool strans_cmp(const struct sip_msg *msg1, const struct sip_msg *msg2)
{
	if (pl_cmp(&msg1->via.branch, &msg2->via.branch))
		return false;

	if (pl_cmp(&msg1->via.sentby, &msg2->via.sentby))
		return false;

	return true;
}


static bool cmp_handler(struct le *le, void *arg)
{
	struct sip_strans *st = le->data;
	const struct sip_msg *msg = arg;

	if (!strans_cmp(st->msg, msg))
		return false;

	if (pl_cmp(&st->msg->cseq.met, &msg->cseq.met))
		return false;

	return true;
}


static bool cmp_ack_handler(struct le *le, void *arg)
{
	struct sip_strans *st = le->data;
	const struct sip_msg *msg = arg;

	if (!strans_cmp(st->msg, msg))
		return false;

	if (pl_strcmp(&st->msg->cseq.met, "INVITE"))
		return false;

	return true;
}


static bool cmp_cancel_handler(struct le *le, void *arg)
{
	struct sip_strans *st = le->data;
	const struct sip_msg *msg = arg;

	if (!strans_cmp(st->msg, msg))
		return false;

	if (!pl_strcmp(&st->msg->cseq.met, "CANCEL"))
		return false;

	return true;
}


static bool cmp_merge_handler(struct le *le, void *arg)
{
	struct sip_strans *st = le->data;
	const struct sip_msg *msg = arg;

	if (pl_cmp(&st->msg->cseq.met, &msg->cseq.met))
		return false;

	if (st->msg->cseq.num != msg->cseq.num)
		return false;

	if (pl_cmp(&st->msg->callid, &msg->callid))
		return false;

	if (pl_cmp(&st->msg->from.tag, &msg->from.tag))
		return false;

	if (pl_cmp(&st->msg->ruri, &msg->ruri))
		return false;

	return true;
}


static void dummy_handler(void *arg)
{
	(void)arg;
}


static void tmr_handler(void *arg)
{
	struct sip_strans *st = arg;

	mem_deref(st);
}


static void retransmit_handler(void *arg)
{
	struct sip_strans *st = arg;

	(void)sip_send(st->sip, st->msg->sock, st->msg->tp, &st->dst,
		       st->mb);

	st->txc++;
	tmr_start(&st->tmrg, MIN(SIP_T1<<st->txc, SIP_T2), retransmit_handler,
		  st);
}


static bool ack_handler(struct sip *sip, const struct sip_msg *msg)
{
	struct sip_strans *st;

	st = list_ledata(hash_lookup(sip->ht_strans,
				     hash_joaat_pl(&msg->via.branch),
				     cmp_ack_handler, (void *)msg));
	if (!st)
		return false;

	switch (st->state) {

	case ACCEPTED:
		/* make sure ACKs for 2xx are passed to TU */
		return false;

	case COMPLETED:
		if (sip_transp_reliable(st->msg->tp)) {
			mem_deref(st);
			break;
		}

		tmr_start(&st->tmr, SIP_T4, tmr_handler, st);
		tmr_cancel(&st->tmrg);
		st->state = CONFIRMED;
		break;

	default:
		break;
	}

	return true;
}


static bool cancel_handler(struct sip *sip, const struct sip_msg *msg)
{
	struct sip_strans *st;

	st = list_ledata(hash_lookup(sip->ht_strans,
				     hash_joaat_pl(&msg->via.branch),
				     cmp_cancel_handler, (void *)msg));
	if (!st)
		return false;

	((struct sip_msg *)msg)->tag = st->msg->tag;

	(void)sip_reply(sip, msg, 200, "OK");

	switch (st->state) {

	case TRYING:
	case PROCEEDING:
		st->cancelh(st->arg);
		break;

	default:
		break;
	}

	return true;
}


static bool request_handler(const struct sip_msg *msg, void *arg)
{
	struct sip_strans *st;
	struct sip *sip = arg;

	if (!pl_strcmp(&msg->met, "ACK"))
		return ack_handler(sip, msg);

	st = list_ledata(hash_lookup(sip->ht_strans,
				     hash_joaat_pl(&msg->via.branch),
				     cmp_handler, (void *)msg));
	if (st) {
		switch (st->state) {

		case PROCEEDING:
		case COMPLETED:
			(void)sip_send(st->sip, st->msg->sock, st->msg->tp,
				       &st->dst, st->mb);
			break;

		default:
			break;
		}

		return true;
	}
	else if (!pl_isset(&msg->to.tag)) {

		st = list_ledata(hash_lookup(sip->ht_strans_mrg,
					     hash_joaat_pl(&msg->callid),
					     cmp_merge_handler, (void *)msg));
		if (st) {
			(void)sip_reply(sip, msg, 482, "Loop Detected");
			return true;
		}
	}

	if (!pl_strcmp(&msg->met, "CANCEL"))
		return cancel_handler(sip, msg);

	return false;
}


/**
 * Allocate a SIP Server Transaction
 *
 * @param stp     Pointer to allocated SIP Server Transaction
 * @param sip     SIP Stack instance
 * @param msg     Incoming SIP message
 * @param cancelh Cancel handler
 * @param arg     Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_strans_alloc(struct sip_strans **stp, struct sip *sip,
		     const struct sip_msg *msg, sip_cancel_h *cancelh,
		     void *arg)
{
	struct sip_strans *st;

	if (!stp || !sip || !msg)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	hash_append(sip->ht_strans, hash_joaat_pl(&msg->via.branch),
		    &st->he, st);

	hash_append(sip->ht_strans_mrg, hash_joaat_pl(&msg->callid),
		    &st->he_mrg, st);

	st->invite  = !pl_strcmp(&msg->met, "INVITE");
	st->msg     = mem_ref((void *)msg);
	st->state   = TRYING;
	st->cancelh = cancelh ? cancelh : dummy_handler;
	st->arg     = arg;
	st->sip     = sip;

	*stp = st;

	return 0;
}


/**
 * Reply using a SIP Server Transaction
 *
 * @param stp   Pointer to allocated SIP Server Transaction
 * @param sip   SIP Stack instance
 * @param msg   Incoming SIP message
 * @param dst   Destination network address
 * @param scode Response status code
 * @param mb    Buffer containing SIP response
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_strans_reply(struct sip_strans **stp, struct sip *sip,
		     const struct sip_msg *msg, const struct sa *dst,
		     uint16_t scode, struct mbuf *mb)
{
	struct sip_strans *st = NULL;
	int err;

	if (!sip || !mb || !dst || (scode < 200 && !stp))
		return EINVAL;

	if (stp)
		st = *stp;

	if (!st) {
		err = sip_strans_alloc(&st, sip, msg, NULL, NULL);
		if (err)
			return err;
	}

	mem_deref(st->mb);
	st->mb = mem_ref(mb);
	st->dst = *dst;

	err = sip_send(sip, st->msg->sock, st->msg->tp, dst, mb);

	if (stp)
		*stp = (err || scode >= 200) ? NULL : st;

	if (err) {
		mem_deref(st);
		return err;
	}

	if (st->invite) {
		if (scode < 200) {
			st->state = PROCEEDING;
		}
		else if (scode < 300) {
			tmr_start(&st->tmr, 64 * SIP_T1, tmr_handler, st);
			st->state = ACCEPTED;
		}
		else {
			tmr_start(&st->tmr, 64 * SIP_T1, tmr_handler, st);
			st->state = COMPLETED;

			if (!sip_transp_reliable(st->msg->tp))
				tmr_start(&st->tmrg, SIP_T1,
					  retransmit_handler, st);
		}
	}
	else {
		if (scode < 200) {
			st->state = PROCEEDING;
		}
		else {
			if (!sip_transp_reliable(st->msg->tp)) {
				tmr_start(&st->tmr, 64 * SIP_T1, tmr_handler,
					  st);
				st->state = COMPLETED;
			}
			else {
				mem_deref(st);
			}
		}
	}

	return 0;
}


int sip_strans_init(struct sip *sip, uint32_t sz)
{
	int err;

	err = sip_listen(NULL, sip, true, request_handler, sip);
	if (err)
		return err;

	err = hash_alloc(&sip->ht_strans_mrg, sz);
	if (err)
		return err;

	return hash_alloc(&sip->ht_strans, sz);
}


static const char *statename(enum state state)
{
	switch (state) {

	case TRYING:     return "TRYING";
	case PROCEEDING: return "PROCEEDING";
	case ACCEPTED:   return "ACCEPTED";
	case COMPLETED:  return "COMPLETED";
	case CONFIRMED:  return "CONFIRMED";
	default:         return "???";
	}
}


static bool debug_handler(struct le *le, void *arg)
{
	struct sip_strans *st = le->data;
	struct re_printf *pf = arg;

	(void)re_hprintf(pf, "  %-10r %-10s %2llus (%r)\n",
			 &st->msg->met,
			 statename(st->state),
			 tmr_get_expire(&st->tmr)/1000,
			 &st->msg->via.branch);

	return false;
}


int sip_strans_debug(struct re_printf *pf, const struct sip *sip)
{
	int err;

	err = re_hprintf(pf, "server transactions:\n");
	hash_apply(sip->ht_strans, debug_handler, pf);

	return err;
}
