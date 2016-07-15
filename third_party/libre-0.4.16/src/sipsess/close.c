/**
 * @file close.c  SIP Session Close
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


static void bye_resp_handler(int err, const struct sip_msg *msg, void *arg)
{
	struct sipsess *sess = arg;

	if (err || sip_request_loops(&sess->ls, msg->scode))
		goto out;

	if (msg->scode < 200) {
		return;
	}
	else if (msg->scode < 300) {
		;
	}
	else {
		if (sess->peerterm)
			goto out;

		switch (msg->scode) {

		case 401:
		case 407:
			err = sip_auth_authenticate(sess->auth, msg);
			if (err)
				break;

			err = sipsess_bye(sess, false);
			if (err)
				break;

			return;
		}
	}

 out:
	mem_deref(sess);
}


int sipsess_bye(struct sipsess *sess, bool reset_ls)
{
	if (sess->req)
		return EPROTO;

	if (reset_ls)
		sip_loopstate_reset(&sess->ls);

	return sip_drequestf(&sess->req, sess->sip, true, "BYE",
			     sess->dlg, 0, sess->auth,
			     NULL, bye_resp_handler, sess,
			     "%s"
			     "Content-Length: 0\r\n"
			     "\r\n",
			     sess->close_hdrs);
}
