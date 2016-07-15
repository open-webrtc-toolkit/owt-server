/**
 * @file modify.c  SIP Session Modify
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


static void tmr_handler(void *arg)
{
	struct sipsess *sess = arg;

	(void)sipsess_reinvite(sess, true);
}


static void reinvite_resp_handler(int err, const struct sip_msg *msg,
				  void *arg)
{
	struct sipsess *sess = arg;
	const struct sip_hdr *hdr;
	struct mbuf *desc = NULL;

	if (err || sip_request_loops(&sess->ls, msg->scode))
		goto out;

	if (msg->scode < 200) {
		return;
	}
	else if (msg->scode < 300) {

		(void)sip_dialog_update(sess->dlg, msg);

		if (sess->sent_offer)
			(void)sess->answerh(msg, sess->arg);
		else {
			sess->modify_pending = false;
			(void)sess->offerh(&desc, msg, sess->arg);
		}

		(void)sipsess_ack(sess->sock, sess->dlg, msg->cseq.num,
				  sess->auth, sess->ctype, desc);
		mem_deref(desc);
	}
	else {
		if (sess->terminated)
			goto out;

		switch (msg->scode) {

		case 401:
		case 407:
			err = sip_auth_authenticate(sess->auth, msg);
			if (err) {
				err = (err == EAUTH) ? 0 : err;
				break;
			}

			err = sipsess_reinvite(sess, false);
			if (err)
				break;

			return;

		case 408:
		case 481:
			sipsess_terminate(sess, 0, msg);
			return;

		case 491:
			tmr_start(&sess->tmr, sess->owner ? 3000 : 1000,
				  tmr_handler, sess);
			return;

		case 500:
			hdr = sip_msg_hdr(msg, SIP_HDR_RETRY_AFTER);
			if (!hdr)
				break;

			tmr_start(&sess->tmr, pl_u32(&hdr->val) * 1000,
				  tmr_handler, sess);
			return;
		}
	}
 out:
	if (sess->terminated)
		mem_deref(sess);
	else if (err == ETIMEDOUT)
		sipsess_terminate(sess, err, NULL);
	else if (sess->modify_pending)
		(void)sipsess_reinvite(sess, true);
	else
		sess->desc = mem_deref(sess->desc);
}


static int send_handler(enum sip_transp tp, const struct sa *src,
			const struct sa *dst, struct mbuf *mb, void *arg)
{
	struct sip_contact contact;
	struct sipsess *sess = arg;
	(void)dst;

	sip_contact_set(&contact, sess->cuser, src, tp);

	return mbuf_printf(mb, "%H", sip_contact_print, &contact);
}


int sipsess_reinvite(struct sipsess *sess, bool reset_ls)
{
	if (sess->req)
		return EPROTO;

	sess->sent_offer = sess->desc ? true : false;
	sess->modify_pending = false;

	if (reset_ls)
		sip_loopstate_reset(&sess->ls);

	return sip_drequestf(&sess->req, sess->sip, true, "INVITE",
			     sess->dlg, 0, sess->auth,
			     send_handler, reinvite_resp_handler, sess,
			     "%s%s%s"
			     "Content-Length: %zu\r\n"
			     "\r\n"
			     "%b",
			     sess->desc ? "Content-Type: " : "",
			     sess->desc ? sess->ctype : "",
			     sess->desc ? "\r\n" : "",
			     sess->desc ? mbuf_get_left(sess->desc) :(size_t)0,
			     sess->desc ? mbuf_buf(sess->desc) : NULL,
			     sess->desc ? mbuf_get_left(sess->desc):(size_t)0);
}


/**
 * Modify an established SIP Session sending Re-INVITE or UPDATE
 *
 * @param sess      SIP Session
 * @param desc      Content description (e.g. SDP)
 *
 * @return 0 if success, otherwise errorcode
 */
int sipsess_modify(struct sipsess *sess, struct mbuf *desc)
{
	if (!sess || sess->st || sess->terminated)
		return EINVAL;

	mem_deref(sess->desc);
	sess->desc = mem_ref(desc);

	if (sess->req || sess->tmr.th || sess->replyl.head) {
		sess->modify_pending = true;
		return 0;
	}

	return sipsess_reinvite(sess, true);
}
