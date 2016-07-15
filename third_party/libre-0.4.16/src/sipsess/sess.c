/**
 * @file sipsess/sess.c  SIP Session Core
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


static int internal_offer_handler(struct mbuf **descp,
				  const struct sip_msg *msg, void *arg)
{
	(void)descp;
	(void)msg;
	(void)arg;

	return ENOSYS;
}


static int internal_answer_handler(const struct sip_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;

	return ENOSYS;
}


static void internal_progress_handler(const struct sip_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;
}


static void internal_establish_handler(const struct sip_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;
}


static void internal_close_handler(int err, const struct sip_msg *msg,
				   void *arg)
{
	(void)err;
	(void)msg;
	(void)arg;
}


static bool termwait(struct sipsess *sess)
{
	bool wait = false;

	sess->terminated = 1;
	sess->offerh  = internal_offer_handler;
	sess->answerh = internal_answer_handler;
	sess->progrh  = internal_progress_handler;
	sess->estabh  = internal_establish_handler;
	sess->infoh   = NULL;
	sess->referh  = NULL;
	sess->closeh  = internal_close_handler;
	sess->arg     = sess;

	tmr_cancel(&sess->tmr);

	if (sess->st) {
		(void)sip_treply(&sess->st, sess->sip, sess->msg,
				 486, "Busy Here");
	}

	if (sess->req) {
		sip_request_cancel(sess->req);
		mem_ref(sess);
		wait = true;
	}

	if (sess->replyl.head) {
		mem_ref(sess);
		wait = true;
	}

	if (sess->requestl.head) {
		mem_ref(sess);
		wait = true;
	}

	return wait;
}


static bool terminate(struct sipsess *sess)
{
	sess->terminated = 2;

	if (!sess->established || sess->peerterm)
		return false;

	if (sipsess_bye(sess, true))
		return false;

	mem_ref(sess);
	return true;
}


static void destructor(void *arg)
{
	struct sipsess *sess = arg;

	switch (sess->terminated) {

	case 0:
		if (termwait(sess))
			return;

		/*@fallthrough@*/

	case 1:
		if (terminate(sess))
			return;
		break;
	}

	hash_unlink(&sess->he);
	tmr_cancel(&sess->tmr);
	list_flush(&sess->replyl);
	list_flush(&sess->requestl);
	mem_deref((void *)sess->msg);
	mem_deref(sess->req);
	mem_deref(sess->dlg);
	mem_deref(sess->auth);
	mem_deref(sess->cuser);
	mem_deref(sess->ctype);
	mem_deref(sess->close_hdrs);
	mem_deref(sess->hdrs);
	mem_deref(sess->desc);
	mem_deref(sess->sock);
	mem_deref(sess->sip);
	mem_deref(sess->st);
}


int sipsess_alloc(struct sipsess **sessp, struct sipsess_sock *sock,
		  const char *cuser, const char *ctype, struct mbuf *desc,
		  sip_auth_h *authh, void *aarg, bool aref,
		  sipsess_offer_h *offerh, sipsess_answer_h *answerh,
		  sipsess_progr_h *progrh, sipsess_estab_h *estabh,
		  sipsess_info_h *infoh, sipsess_refer_h *referh,
		  sipsess_close_h *closeh, void *arg)
{
	struct sipsess *sess;
	int err;

	sess = mem_zalloc(sizeof(*sess), destructor);
	if (!sess)
		return ENOMEM;

	err = sip_auth_alloc(&sess->auth, authh, aarg, aref);
	if (err)
		goto out;

	err = str_dup(&sess->cuser, cuser);
	if (err)
		goto out;

	err = str_dup(&sess->ctype, ctype);
	if (err)
		goto out;

	sess->sock    = mem_ref(sock);
	sess->desc    = mem_ref(desc);
	sess->sip     = mem_ref(sock->sip);
	sess->offerh  = offerh  ? offerh  : internal_offer_handler;
	sess->answerh = answerh ? answerh : internal_answer_handler;
	sess->progrh  = progrh  ? progrh  : internal_progress_handler;
	sess->estabh  = estabh  ? estabh  : internal_establish_handler;
	sess->infoh   = infoh;
	sess->referh  = referh;
	sess->closeh  = closeh  ? closeh  : internal_close_handler;
	sess->arg     = arg;

 out:
	if (err)
		mem_deref(sess);
	else
		*sessp = sess;

	return err;
}


void sipsess_terminate(struct sipsess *sess, int err,
		       const struct sip_msg *msg)
{
	sipsess_close_h *closeh;
	void *arg;

	if (sess->terminated)
		return;

	closeh = sess->closeh;
	arg    = sess->arg;

	if (!termwait(sess))
		(void)terminate(sess);

	closeh(err, msg, arg);
}


/**
 * Get the SIP dialog from a SIP Session
 *
 * @param sess      SIP Session
 *
 * @return SIP Dialog object
 */
struct sip_dialog *sipsess_dialog(const struct sipsess *sess)
{
	return sess ? sess->dlg : NULL;
}


/**
 * Set extra SIP headers for inclusion in Session "close" messages
 * like BYE and 200 OK. Multiple headers can be included.
 *
 * @param sess      SIP Session
 * @param hdrs      Formatted strings with extra SIP Headers
 *
 * @return 0 if success, otherwise errorcode
 */
int sipsess_set_close_headers(struct sipsess *sess, const char *hdrs, ...)
{
	int err = 0;
	va_list ap;

	if (!sess)
		return EINVAL;

	sess->close_hdrs = mem_deref(sess->close_hdrs);

	if (hdrs) {
		va_start(ap, hdrs);
		err = re_vsdprintf(&sess->close_hdrs, hdrs, ap);
		va_end(ap);
	}

	return err;
}
