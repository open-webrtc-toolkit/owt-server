/**
 * @file bfcp/request.c BFCP Request
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_tmr.h>
#include <re_bfcp.h>
#include "bfcp.h"


enum {
	BFCP_T1  = 500,
	BFCP_TXC = 4,
};


struct bfcp_ctrans {
	struct le le;
	struct sa dst;
	struct mbuf *mb;
	bfcp_resp_h *resph;
	void *arg;
	uint32_t confid;
	uint16_t userid;
	uint16_t tid;
};


static void tmr_handler(void *arg);


static void dummy_resp_handler(int err, const struct bfcp_msg *msg, void *arg)
{
	(void)err;
	(void)msg;
	(void)arg;
}


static void destructor(void *arg)
{
	struct bfcp_ctrans *ct = arg;

	list_unlink(&ct->le);
	mem_deref(ct->mb);
}


static void dispatch(struct bfcp_conn *bc)
{
	struct le *le = bc->ctransl.head;

	while (le) {
		struct bfcp_ctrans *ct = le->data;
		int err;

		le = le->next;

		err = bfcp_send(bc, &ct->dst, ct->mb);
		if (err) {
			ct->resph(err, NULL, ct->arg);
			mem_deref(ct);
			continue;
		}

		tmr_start(&bc->tmr1, BFCP_T1, tmr_handler, bc);
		bc->txc = 1;
		break;
	}
}


static void tmr_handler(void *arg)
{
	struct bfcp_conn *bc = arg;
	struct bfcp_ctrans *ct;
	uint32_t timeout;
	int err;

	ct = list_ledata(bc->ctransl.head);
	if (!ct)
		return;

	timeout = BFCP_T1<<bc->txc;

	if (++bc->txc > BFCP_TXC) {
		err = ETIMEDOUT;
		goto out;
	}

	err = bfcp_send(bc, &ct->dst, ct->mb);
	if (err)
		goto out;

	tmr_start(&bc->tmr1, timeout, tmr_handler, bc);
	return;

 out:
	ct->resph(err, NULL, ct->arg);
	mem_deref(ct);
	dispatch(bc);
}


bool bfcp_handle_response(struct bfcp_conn *bc, const struct bfcp_msg *msg)
{
	struct bfcp_ctrans *ct;

	if (!bc || !msg)
		return false;

	ct = list_ledata(bc->ctransl.head);
	if (!ct)
		return false;

	if (msg->tid != ct->tid)
		return false;

	if (msg->confid != ct->confid)
		return false;

	if (msg->userid != ct->userid)
		return false;

	tmr_cancel(&bc->tmr1);

	ct->resph(0, msg, ct->arg);
	mem_deref(ct);

	dispatch(bc);

	return true;
}


int bfcp_vrequest(struct bfcp_conn *bc, const struct sa *dst, uint8_t ver,
		  enum bfcp_prim prim, uint32_t confid, uint16_t userid,
		  bfcp_resp_h *resph, void *arg, unsigned attrc, va_list *ap)
{
	struct bfcp_ctrans *ct;
	int err;

	if (!bc || !dst)
		return EINVAL;

	ct = mem_zalloc(sizeof(*ct), destructor);
	if (!ct)
		return ENOMEM;

	if (bc->tid == 0)
		bc->tid = 1;

	ct->dst    = *dst;
	ct->confid = confid;
	ct->userid = userid;
	ct->tid    = bc->tid++;
	ct->resph  = resph ? resph : dummy_resp_handler;
	ct->arg    = arg;

	ct->mb = mbuf_alloc(128);
	if (!ct->mb) {
		err = ENOMEM;
		goto out;
	}

	err = bfcp_msg_vencode(ct->mb, ver, false, prim, confid, ct->tid,
			       userid, attrc, ap);
	if (err)
		goto out;

	ct->mb->pos = 0;

	if (!bc->ctransl.head) {

		err = bfcp_send(bc, &ct->dst, ct->mb);
		if (err)
			goto out;

		tmr_start(&bc->tmr1, BFCP_T1, tmr_handler, bc);
		bc->txc = 1;
	}

	list_append(&bc->ctransl, &ct->le, ct);

 out:
	if (err)
		mem_deref(ct);

	return err;
}


/**
 * Send a BFCP request
 *
 * @param bc      BFCP connection
 * @param dst     Destination address
 * @param ver     BFCP Version
 * @param prim    BFCP Primitive
 * @param confid  Conference ID
 * @param userid  User ID
 * @param resph   Response handler
 * @param arg     Response handler argument
 * @param attrc   Number of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_request(struct bfcp_conn *bc, const struct sa *dst, uint8_t ver,
		 enum bfcp_prim prim, uint32_t confid, uint16_t userid,
		 bfcp_resp_h *resph, void *arg, unsigned attrc, ...)
{
	va_list ap;
	int err;

	va_start(ap, attrc);
	err = bfcp_vrequest(bc, dst, ver, prim, confid, userid, resph, arg,
			    attrc, &ap);
	va_end(ap);

	return err;
}


/**
 * Send a BFCP notification/subsequent response
 *
 * @param bc      BFCP connection
 * @param dst     Destination address
 * @param ver     BFCP Version
 * @param prim    BFCP Primitive
 * @param confid  Conference ID
 * @param userid  User ID
 * @param attrc   Number of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_notify(struct bfcp_conn *bc, const struct sa *dst, uint8_t ver,
		enum bfcp_prim prim, uint32_t confid, uint16_t userid,
		unsigned attrc, ...)
{
	va_list ap;
	int err;

	va_start(ap, attrc);
	err = bfcp_vrequest(bc, dst, ver, prim, confid, userid, NULL, NULL,
			    attrc, &ap);
	va_end(ap);

	return err;
}
