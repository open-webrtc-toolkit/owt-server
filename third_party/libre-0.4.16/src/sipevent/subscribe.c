/**
 * @file subscribe.c  SIP Event Subscribe
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
#include <re_msg.h>
#include <re_sip.h>
#include <re_sipevent.h>
#include "sipevent.h"


enum {
	DEFAULT_EXPIRES = 3600,
	RESUB_FAIL_WAIT = 60000,
	RESUB_FAILC_MAX = 7,
	NOTIFY_TIMEOUT  = 10000,
};


static int request(struct sipsub *sub, bool reset_ls);


static void internal_notify_handler(struct sip *sip, const struct sip_msg *msg,
				    void *arg)
{
	(void)arg;

	(void)sip_treply(NULL, sip, msg, 200, "OK");
}


static void internal_close_handler(int err, const struct sip_msg *msg,
				   const struct sipevent_substate *substate,
				   void *arg)
{
	(void)err;
	(void)msg;
	(void)substate;
	(void)arg;
}


static bool terminate(struct sipsub *sub)
{
	sub->terminated = true;
	sub->forkh      = NULL;
	sub->notifyh    = internal_notify_handler;
	sub->closeh     = internal_close_handler;

	if (sub->termwait) {
		mem_ref(sub);
		return true;
	}

	tmr_cancel(&sub->tmr);

	if (sub->req) {
		mem_ref(sub);
		return true;
	}

	if (sub->expires && sub->subscribed && !request(sub, true)) {
		mem_ref(sub);
		return true;
	}

	return false;
}


static void destructor(void *arg)
{
	struct sipsub *sub = arg;

	if (!sub->terminated) {

		if (terminate(sub))
			return;
	}

	tmr_cancel(&sub->tmr);
	hash_unlink(&sub->he);
	mem_deref(sub->req);
	mem_deref(sub->dlg);
	mem_deref(sub->auth);
	mem_deref(sub->event);
	mem_deref(sub->id);
	mem_deref(sub->cuser);
	mem_deref(sub->hdrs);
	mem_deref(sub->refer_hdrs);
	mem_deref(sub->sock);
	mem_deref(sub->sip);
}


static void notify_timeout_handler(void *arg)
{
	struct sipsub *sub = arg;

	sub->termwait = false;

	if (sub->terminated)
		mem_deref(sub);
	else
		sipsub_terminate(sub, ETIMEDOUT, NULL, NULL);
}


static void tmr_handler(void *arg)
{
	struct sipsub *sub = arg;
	int err;

	if (sub->req || sub->terminated)
		return;

	err = request(sub, true);
	if (err) {
		if (++sub->failc < RESUB_FAILC_MAX) {
			sipsub_reschedule(sub, RESUB_FAIL_WAIT);
		}
		else {
			sipsub_terminate(sub, err, NULL, NULL);
		}
	}
}


void sipsub_reschedule(struct sipsub *sub, uint64_t wait)
{
	tmr_start(&sub->tmr, wait, tmr_handler, sub);
}


void sipsub_terminate(struct sipsub *sub, int err, const struct sip_msg *msg,
		      const struct sipevent_substate *substate)
{
	sipsub_close_h *closeh;
	void *arg;

	closeh = sub->closeh;
	arg    = sub->arg;

	(void)terminate(sub);

	closeh(err, msg, substate, arg);
}


static void response_handler(int err, const struct sip_msg *msg, void *arg)
{
	const struct sip_hdr *minexp;
	struct sipsub *sub = arg;

	if (err || sip_request_loops(&sub->ls, msg->scode))
		goto out;

	if (msg->scode < 200) {
		return;
	}
	else if (msg->scode < 300) {

		uint32_t wait;

		if (sub->forkh) {

			struct sipsub *fsub;

			fsub = sipsub_find(sub->sock, msg, NULL, true);
			if (!fsub) {

				err = sub->forkh(&fsub, sub, msg, sub->arg);
				if (err)
					return;
			}
			else {
				(void)sip_dialog_update(fsub->dlg, msg);
			}

			sub = fsub;
		}
		else if (!sip_dialog_established(sub->dlg)) {

			err = sip_dialog_create(sub->dlg, msg);
			if (err) {
				sub->subscribed = false;
				goto out;
			}
		}
		else {
			/* Ignore 2xx responses for other dialogs
			 * if forking is disabled */
			if (!sip_dialog_cmp(sub->dlg, msg))
				return;

			(void)sip_dialog_update(sub->dlg, msg);
		}

		if (!sub->termconf)
			sub->subscribed = true;

		sub->failc = 0;

		if (!sub->expires && !sub->termconf) {

			tmr_start(&sub->tmr, NOTIFY_TIMEOUT,
				  notify_timeout_handler, sub);
			sub->termwait = true;
			return;
		}

		if (sub->terminated)
			goto out;

		if (sub->refer) {
			sub->refer = false;
			return;
		}

		if (pl_isset(&msg->expires))
			wait = pl_u32(&msg->expires);
		else
			wait = sub->expires;

		sipsub_reschedule(sub, wait * 900);
		return;
	}
	else {
		if (sub->terminated && !sub->subscribed)
			goto out;

		switch (msg->scode) {

		case 401:
		case 407:
			err = sip_auth_authenticate(sub->auth, msg);
			if (err) {
				err = (err == EAUTH) ? 0 : err;
				break;
			}

			err = request(sub, false);
			if (err)
				break;

			return;

		case 403:
			sip_auth_reset(sub->auth);
			break;

		case 423:
			minexp = sip_msg_hdr(msg, SIP_HDR_MIN_EXPIRES);
			if (!minexp || !pl_u32(&minexp->val) || !sub->expires)
				break;

			sub->expires = pl_u32(&minexp->val);

			err = request(sub, false);
			if (err)
				break;

			return;

		case 481:
			sub->subscribed = false;
			break;
		}
	}

 out:
	sub->refer = false;

	if (sub->terminated) {

		if (!sub->expires || !sub->subscribed || request(sub, true))
			mem_deref(sub);
	}
	else {
		if (sub->subscribed && ++sub->failc < RESUB_FAILC_MAX)
			sipsub_reschedule(sub, RESUB_FAIL_WAIT);
		else
			sipsub_terminate(sub, err, msg, NULL);
	}
}


static int send_handler(enum sip_transp tp, const struct sa *src,
			const struct sa *dst, struct mbuf *mb, void *arg)
{
	struct sip_contact contact;
	struct sipsub *sub = arg;
	(void)dst;

	sip_contact_set(&contact, sub->cuser, src, tp);

	return mbuf_printf(mb, "%H", sip_contact_print, &contact);
}


static int print_event(struct re_printf *pf, const struct sipsub *sub)
{
	if (sub->id)
		return re_hprintf(pf, "%s;id=%s", sub->event, sub->id);
	else
		return re_hprintf(pf, "%s", sub->event);
}


static int request(struct sipsub *sub, bool reset_ls)
{
	if (reset_ls)
		sip_loopstate_reset(&sub->ls);

	if (sub->refer) {

		sub->refer_cseq = sip_dialog_lseq(sub->dlg);

		return sip_drequestf(&sub->req, sub->sip, true, "REFER",
				     sub->dlg, 0, sub->auth,
				     send_handler, response_handler, sub,
				     "%s"
				     "Content-Length: 0\r\n"
				     "\r\n",
				     sub->refer_hdrs);
	}
	else {
		if (sub->terminated)
			sub->expires = 0;

		return sip_drequestf(&sub->req, sub->sip, true, "SUBSCRIBE",
				     sub->dlg, 0, sub->auth,
				     send_handler, response_handler, sub,
				     "Event: %H\r\n"
				     "Expires: %u\r\n"
				     "%s"
				     "Content-Length: 0\r\n"
				     "\r\n",
				     print_event, sub,
				     sub->expires,
				     sub->hdrs);
	}
}


static int sipsub_alloc(struct sipsub **subp, struct sipevent_sock *sock,
			bool refer, struct sip_dialog *dlg, const char *uri,
			const char *from_name, const char *from_uri,
			const char *event, const char *id, uint32_t expires,
			const char *cuser,
			const char *routev[], uint32_t routec,
			sip_auth_h *authh, void *aarg, bool aref,
			sipsub_fork_h *forkh, sipsub_notify_h *notifyh,
			sipsub_close_h *closeh, void *arg,
			const char *fmt, va_list ap)
{
	struct sipsub *sub;
	int err;

	if (!subp || !sock || !event || !cuser)
		return EINVAL;

	if (!dlg && (!uri || !from_uri))
		return EINVAL;

	sub = mem_zalloc(sizeof(*sub), destructor);
	if (!sub)
		return ENOMEM;

	if (dlg) {
		sub->dlg = mem_ref(dlg);
	}
	else {
		err = sip_dialog_alloc(&sub->dlg, uri, uri, from_name,
				       from_uri, routev, routec);
		if (err)
			goto out;
	}

	hash_append(sock->ht_sub,
		    hash_joaat_str(sip_dialog_callid(sub->dlg)),
		    &sub->he, sub);

	err = sip_auth_alloc(&sub->auth, authh, aarg, aref);
	if (err)
		goto out;

	err = str_dup(&sub->event, event);
	if (err)
		goto out;

	if (id) {
		err = str_dup(&sub->id, id);
		if (err)
			goto out;
	}

	err = str_dup(&sub->cuser, cuser);
	if (err)
		goto out;

	if (fmt) {
		err = re_vsdprintf(refer ? &sub->refer_hdrs : &sub->hdrs,
				   fmt, ap);
		if (err)
			goto out;
	}

	sub->refer_cseq = -1;
	sub->refer   = refer;
	sub->sock    = mem_ref(sock);
	sub->sip     = mem_ref(sock->sip);
	sub->expires = expires;
	sub->forkh   = forkh;
	sub->notifyh = notifyh ? notifyh : internal_notify_handler;
	sub->closeh  = closeh  ? closeh  : internal_close_handler;
	sub->arg     = arg;

	err = request(sub, true);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(sub);
	else
		*subp = sub;

	return err;
}


/**
 * Allocate a SIP subscriber client
 *
 * @param subp      Pointer to allocated SIP subscriber client
 * @param sock      SIP Event socket
 * @param uri       SIP Request URI
 * @param from_name SIP From-header Name (optional)
 * @param from_uri  SIP From-header URI
 * @param event     SIP Event to subscribe to
 * @param id        SIP Event ID (optional)
 * @param expires   Subscription expires value
 * @param cuser     Contact username or URI
 * @param routev    Optional route vector
 * @param routec    Number of routes
 * @param authh     Authentication handler
 * @param aarg      Authentication handler argument
 * @param aref      True to ref argument
 * @param forkh     Fork handler
 * @param notifyh   Notify handler
 * @param closeh    Close handler
 * @param arg       Response handler argument
 * @param fmt       Formatted strings with extra SIP Headers
 *
 * @return 0 if success, otherwise errorcode
 */
int sipevent_subscribe(struct sipsub **subp, struct sipevent_sock *sock,
		       const char *uri, const char *from_name,
		       const char *from_uri, const char *event, const char *id,
		       uint32_t expires, const char *cuser,
		       const char *routev[], uint32_t routec,
		       sip_auth_h *authh, void *aarg, bool aref,
		       sipsub_fork_h *forkh, sipsub_notify_h *notifyh,
		       sipsub_close_h *closeh, void *arg,
		       const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = sipsub_alloc(subp, sock, false, NULL, uri, from_name, from_uri,
			   event, id, expires, cuser,
			   routev, routec, authh, aarg, aref, forkh, notifyh,
			   closeh, arg, fmt, ap);
	va_end(ap);

	return err;
}


/**
 * Allocate a SIP subscriber client using an existing dialog
 *
 * @param subp      Pointer to allocated SIP subscriber client
 * @param sock      SIP Event socket
 * @param dlg       Established SIP Dialog
 * @param event     SIP Event to subscribe to
 * @param id        SIP Event ID (optional)
 * @param expires   Subscription expires value
 * @param cuser     Contact username or URI
 * @param authh     Authentication handler
 * @param aarg      Authentication handler argument
 * @param aref      True to ref argument
 * @param notifyh   Notify handler
 * @param closeh    Close handler
 * @param arg       Response handler argument
 * @param fmt       Formatted strings with extra SIP Headers
 *
 * @return 0 if success, otherwise errorcode
 */
int sipevent_dsubscribe(struct sipsub **subp, struct sipevent_sock *sock,
			struct sip_dialog *dlg, const char *event,
			const char *id, uint32_t expires, const char *cuser,
			sip_auth_h *authh, void *aarg, bool aref,
			sipsub_notify_h *notifyh, sipsub_close_h *closeh,
			void *arg, const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = sipsub_alloc(subp, sock, false, dlg, NULL, NULL, NULL,
			   event, id, expires, cuser,
			   NULL, 0, authh, aarg, aref, NULL, notifyh,
			   closeh, arg, fmt, ap);
	va_end(ap);

	return err;
}


/**
 * Allocate a SIP refer client
 *
 * @param subp      Pointer to allocated SIP subscriber client
 * @param sock      SIP Event socket
 * @param uri       SIP Request URI
 * @param from_name SIP From-header Name (optional)
 * @param from_uri  SIP From-header URI
 * @param cuser     Contact username or URI
 * @param routev    Optional route vector
 * @param routec    Number of routes
 * @param authh     Authentication handler
 * @param aarg      Authentication handler argument
 * @param aref      True to ref argument
 * @param forkh     Fork handler
 * @param notifyh   Notify handler
 * @param closeh    Close handler
 * @param arg       Response handler argument
 * @param fmt       Formatted strings with extra SIP Headers
 *
 * @return 0 if success, otherwise errorcode
 */
int sipevent_refer(struct sipsub **subp, struct sipevent_sock *sock,
		   const char *uri, const char *from_name,
		   const char *from_uri, const char *cuser,
		   const char *routev[], uint32_t routec,
		   sip_auth_h *authh, void *aarg, bool aref,
		   sipsub_fork_h *forkh, sipsub_notify_h *notifyh,
		   sipsub_close_h *closeh, void *arg,
		   const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = sipsub_alloc(subp, sock, true, NULL, uri, from_name, from_uri,
			   "refer", NULL, DEFAULT_EXPIRES, cuser,
			   routev, routec, authh, aarg, aref, forkh, notifyh,
			   closeh, arg, fmt, ap);
	va_end(ap);

	return err;
}


/**
 * Allocate a SIP refer client using an existing dialog
 *
 * @param subp      Pointer to allocated SIP subscriber client
 * @param sock      SIP Event socket
 * @param dlg       Established SIP Dialog
 * @param cuser     Contact username or URI
 * @param authh     Authentication handler
 * @param aarg      Authentication handler argument
 * @param aref      True to ref argument
 * @param notifyh   Notify handler
 * @param closeh    Close handler
 * @param arg       Response handler argument
 * @param fmt       Formatted strings with extra SIP Headers
 *
 * @return 0 if success, otherwise errorcode
 */
int sipevent_drefer(struct sipsub **subp, struct sipevent_sock *sock,
		    struct sip_dialog *dlg, const char *cuser,
		    sip_auth_h *authh, void *aarg, bool aref,
		    sipsub_notify_h *notifyh, sipsub_close_h *closeh,
		    void *arg, const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = sipsub_alloc(subp, sock, true, dlg, NULL, NULL, NULL,
			   "refer", NULL, DEFAULT_EXPIRES, cuser,
			   NULL, 0, authh, aarg, aref, NULL, notifyh,
			   closeh, arg, fmt, ap);
	va_end(ap);

	return err;
}


int sipevent_fork(struct sipsub **subp, struct sipsub *osub,
		  const struct sip_msg *msg,
		  sip_auth_h *authh, void *aarg, bool aref,
		  sipsub_notify_h *notifyh, sipsub_close_h *closeh,
		  void *arg)
{
	struct sipsub *sub;
	int err;

	if (!subp || !osub || !msg)
		return EINVAL;

	sub = mem_zalloc(sizeof(*sub), destructor);
	if (!sub)
		return ENOMEM;

	err = sip_dialog_fork(&sub->dlg, osub->dlg, msg);
	if (err)
		goto out;

	hash_append(osub->sock->ht_sub,
		    hash_joaat_str(sip_dialog_callid(sub->dlg)),
		    &sub->he, sub);

	err = sip_auth_alloc(&sub->auth, authh, aarg, aref);
	if (err)
		goto out;

	sub->event   = mem_ref(osub->event);
	sub->id      = mem_ref(osub->id);
	sub->cuser   = mem_ref(osub->cuser);
	sub->hdrs    = mem_ref(osub->hdrs);
	sub->refer   = osub->refer;
	sub->sock    = mem_ref(osub->sock);
	sub->sip     = mem_ref(osub->sip);
	sub->expires = osub->expires;
	sub->forkh   = NULL;
	sub->notifyh = notifyh ? notifyh : internal_notify_handler;
	sub->closeh  = closeh  ? closeh  : internal_close_handler;
	sub->arg     = arg;

	if (!sub->expires) {
		tmr_start(&sub->tmr, NOTIFY_TIMEOUT,
			  notify_timeout_handler, sub);
		sub->termwait = true;
	}

 out:
	if (err)
		mem_deref(sub);
	else
		*subp = sub;

	return err;
}
