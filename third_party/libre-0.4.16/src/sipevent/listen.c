/**
 * @file sipevent/listen.c  SIP Event Listen
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
#include <re_sipevent.h>
#include "sipevent.h"


struct subcmp {
	const struct sipevent_event *evt;
	const struct sip_msg *msg;
};


static void destructor(void *arg)
{
	struct sipevent_sock *sock = arg;

	mem_deref(sock->lsnr);
	hash_flush(sock->ht_not);
	hash_flush(sock->ht_sub);
	mem_deref(sock->ht_not);
	mem_deref(sock->ht_sub);
}


static bool event_cmp(const struct sipevent_event *evt,
		      const char *event, const char *id,
		      int32_t refer_cseq)
{
	if (pl_strcmp(&evt->event, event))
		return false;

	if (!pl_isset(&evt->id) && !id)
		return true;

	if (!pl_isset(&evt->id))
		return false;

	if (!id) {
		if (refer_cseq >= 0 && (int32_t)pl_u32(&evt->id) == refer_cseq)
			return true;

		return false;
	}

	if (pl_strcmp(&evt->id, id))
		return false;

	return true;
}


static bool not_cmp_handler(struct le *le, void *arg)
{
	const struct subcmp *cmp = arg;
	struct sipnot *not = le->data;

	return sip_dialog_cmp(not->dlg, cmp->msg) &&
		event_cmp(cmp->evt, not->event, not->id, -1);
}


static bool sub_cmp_handler(struct le *le, void *arg)
{
	const struct subcmp *cmp = arg;
	struct sipsub *sub = le->data;

	return sip_dialog_cmp(sub->dlg, cmp->msg) &&
		(!cmp->evt || event_cmp(cmp->evt, sub->event, sub->id,
					sub->refer_cseq));
}


static bool sub_cmp_half_handler(struct le *le, void *arg)
{
	const struct subcmp *cmp = arg;
	struct sipsub *sub = le->data;

	return sip_dialog_cmp_half(sub->dlg, cmp->msg) &&
		!sip_dialog_established(sub->dlg) &&
		(!cmp->evt || event_cmp(cmp->evt, sub->event, sub->id,
					sub->refer_cseq));
}


static struct sipnot *sipnot_find(struct sipevent_sock *sock,
				  const struct sip_msg *msg,
				  const struct sipevent_event *evt)
{
	struct subcmp cmp;

	cmp.msg = msg;
	cmp.evt = evt;

	return list_ledata(hash_lookup(sock->ht_not,
				       hash_joaat_pl(&msg->callid),
				       not_cmp_handler, &cmp));
}


struct sipsub *sipsub_find(struct sipevent_sock *sock,
			   const struct sip_msg *msg,
			   const struct sipevent_event *evt, bool full)
{
	struct subcmp cmp;

	cmp.msg = msg;
	cmp.evt = evt;

	return list_ledata(hash_lookup(sock->ht_sub,
				       hash_joaat_pl(&msg->callid), full ?
				       sub_cmp_handler : sub_cmp_half_handler,
				       &cmp));
}


static void notify_handler(struct sipevent_sock *sock,
			   const struct sip_msg *msg)
{
	struct sipevent_substate state;
	struct sipevent_event event;
	struct sip *sip = sock->sip;
	const struct sip_hdr *hdr;
	struct sipsub *sub;
	uint32_t nrefs;
	char m[256];
	int err;

	hdr = sip_msg_hdr(msg, SIP_HDR_EVENT);
	if (!hdr || sipevent_event_decode(&event, &hdr->val)) {
		(void)sip_reply(sip, msg, 489, "Bad Event");
		return;
	}

	hdr = sip_msg_hdr(msg, SIP_HDR_SUBSCRIPTION_STATE);
	if (!hdr || sipevent_substate_decode(&state, &hdr->val)) {
		(void)sip_reply(sip, msg, 400,"Bad Subscription-State Header");
		return;
	}

	sub = sipsub_find(sock, msg, &event, true);
	if (!sub) {
		sub = sipsub_find(sock, msg, &event, false);
		if (!sub) {
			(void)sip_reply(sip, msg,
					481, "Subscription Does Not Exist");
			return;
		}

		if (sub->forkh) {

			struct sipsub *fsub;

			err = sub->forkh(&fsub, sub, msg, sub->arg);
			if (err) {
				(void)sip_reply(sip, msg, 500,
						str_error(err, m, sizeof(m)));
				return;
			}

			sub = fsub;
		}
		else {
			err = sip_dialog_create(sub->dlg, msg);
			if (err) {
				(void)sip_reply(sip, msg, 500,
						str_error(err, m, sizeof(m)));
				return;
			}
		}
	}
	else {
		if (!sip_dialog_rseq_valid(sub->dlg, msg)) {
			(void)sip_reply(sip, msg, 500, "Bad Sequence");
			return;
		}

		(void)sip_dialog_update(sub->dlg, msg);
	}

	if (sub->refer_cseq >= 0 && !sub->id && pl_isset(&event.id)) {

		err = pl_strdup(&sub->id, &event.id);
		if (err) {
			(void)sip_treply(NULL, sip, msg, 500,
					 str_error(err, m, sizeof(m)));
			return;
		}
	}

	switch (state.state) {

	case SIPEVENT_ACTIVE:
	case SIPEVENT_PENDING:
		if (!sub->termconf)
			sub->subscribed = true;

		if (!sub->terminated && !sub->termwait &&
		    pl_isset(&state.expires))
			sipsub_reschedule(sub, pl_u32(&state.expires) * 900);
		break;

	case SIPEVENT_TERMINATED:
		sub->subscribed = false;
		sub->termconf = true;
		break;
	}

	mem_ref(sub);
	sub->notifyh(sip, msg, sub->arg);
	nrefs = mem_nrefs(sub);
	mem_deref(sub);

	/* check if subscription was deref'd from notify handler */
	if (nrefs == 1)
		return;

	if (state.state == SIPEVENT_TERMINATED) {

		if (!sub->terminated) {
			sub->termwait = false;
			sipsub_terminate(sub, 0, msg, &state);
		}
		else if (sub->termwait) {
			sub->termwait = false;
			tmr_cancel(&sub->tmr);
			mem_deref(sub);
		}
	}
}


static void subscribe_handler(struct sipevent_sock *sock,
			      const struct sip_msg *msg)
{
	struct sipevent_event event;
	struct sip *sip = sock->sip;
	const struct sip_hdr *hdr;
	struct sipnot *not;
	uint32_t expires;

	hdr = sip_msg_hdr(msg, SIP_HDR_EVENT);
	if (!hdr || sipevent_event_decode(&event, &hdr->val)) {
		(void)sip_reply(sip, msg, 400, "Bad Event Header");
		return;
	}

	not = sipnot_find(sock, msg, &event);
	if (!not || not->terminated) {
		(void)sip_reply(sip, msg, 481, "Subscription Does Not Exist");
		return;
	}

	if (pl_isset(&msg->expires))
		expires = pl_u32(&msg->expires);
	else
		expires = not->expires_dfl;

	if (expires > 0 && expires < not->expires_min) {
		(void)sip_replyf(sip, msg, 423, "Interval Too Brief",
				 "Min-Expires: %u\r\n"
				 "Content-Length: 0\r\n"
				 "\r\n",
				 not->expires_min);
		return;
	}

	if (!sip_dialog_rseq_valid(not->dlg, msg)) {
		(void)sip_reply(sip, msg, 500, "Bad Sequence");
		return;
	}

	(void)sip_dialog_update(not->dlg, msg);

	sipnot_refresh(not, expires);

	(void)sipnot_reply(not, msg, 200, "OK");

	(void)sipnot_notify(not);
}


static bool request_handler(const struct sip_msg *msg, void *arg)
{
	struct sipevent_sock *sock = arg;

	if (!pl_strcmp(&msg->met, "SUBSCRIBE")) {

		if (pl_isset(&msg->to.tag)) {
			subscribe_handler(sock, msg);
			return true;
		}

		return sock->subh ? sock->subh(msg, sock->arg) : false;
	}
	else if (!pl_strcmp(&msg->met, "NOTIFY")) {

		notify_handler(sock, msg);
		return true;
	}
	else {
		return false;
	}
}


int sipevent_listen(struct sipevent_sock **sockp, struct sip *sip,
		    uint32_t htsize_not, uint32_t htsize_sub,
		    sip_msg_h *subh, void *arg)
{
	struct sipevent_sock *sock;
	int err;

	if (!sockp || !sip || !htsize_not || !htsize_sub)
		return EINVAL;

	sock = mem_zalloc(sizeof(*sock), destructor);
	if (!sock)
		return ENOMEM;

	err = sip_listen(&sock->lsnr, sip, true, request_handler, sock);
	if (err)
		goto out;

	err = hash_alloc(&sock->ht_not, htsize_not);
	if (err)
		goto out;

	err = hash_alloc(&sock->ht_sub, htsize_sub);
	if (err)
		goto out;

	sock->sip  = sip;
	sock->subh = subh;
	sock->arg  = arg;

 out:
	if (err)
		mem_deref(sock);
	else
		*sockp = sock;

	return err;
}
