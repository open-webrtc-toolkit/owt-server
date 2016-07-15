/**
 * @file info.c  SIP Session Info
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


static int info_request(struct sipsess_request *req);


static void info_resp_handler(int err, const struct sip_msg *msg, void *arg)
{
	struct sipsess_request *req = arg;

	if (err || sip_request_loops(&req->ls, msg->scode))
		goto out;

	if (msg->scode < 200) {
		return;
	}
	else if (msg->scode < 300) {
		;
	}
	else {
		if (req->sess->terminated)
			goto out;

		switch (msg->scode) {

		case 401:
		case 407:
			err = sip_auth_authenticate(req->sess->auth, msg);
			if (err) {
				err = (err == EAUTH) ? 0 : err;
				break;
			}

			err = info_request(req);
			if (err)
				break;

			return;

		case 408:
		case 481:
			sipsess_terminate(req->sess, 0, msg);
			break;
		}
	}

 out:
	if (!req->sess->terminated) {
		if (err == ETIMEDOUT)
			sipsess_terminate(req->sess, err, NULL);
		else
			req->resph(err, msg, req->arg);
	}

	mem_deref(req);
}


static int info_request(struct sipsess_request *req)
{
	return sip_drequestf(&req->req, req->sess->sip, true, "INFO",
			     req->sess->dlg, 0, req->sess->auth,
			     NULL, info_resp_handler, req,
			     "Content-Type: %s\r\n"
			     "Content-Length: %zu\r\n"
			     "\r\n"
			     "%b",
			     req->ctype,
			     mbuf_get_left(req->body),
			     mbuf_buf(req->body), mbuf_get_left(req->body));
}


/**
 * Send a SIP INFO request in the SIP Session
 *
 * @param sess      SIP Session
 * @param ctype     Content-type
 * @param body      Content description (e.g. SDP)
 * @param resph     Response handler
 * @param arg       Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sipsess_info(struct sipsess *sess, const char *ctype, struct mbuf *body,
		 sip_resp_h *resph, void *arg)
{
	struct sipsess_request *req;
	int err;

	if (!sess || sess->terminated || !ctype || !body)
		return EINVAL;

	if (!sip_dialog_established(sess->dlg))
		return ENOTCONN;

	err = sipsess_request_alloc(&req, sess, ctype, body, resph, arg);
	if (err)
		return err;

	err = info_request(req);
	if (err)
		mem_deref(req);

	return err;
}
