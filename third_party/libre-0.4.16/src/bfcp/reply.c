/**
 * @file bfcp/reply.c BFCP Reply
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
	BFCP_T2  = 10000,
};


static void tmr_handler(void *arg)
{
	struct bfcp_conn *bc = arg;

	bc->mb = mem_deref(bc->mb);
}


/**
 * Send a BFCP response
 *
 * @param bc      BFCP connection
 * @param req     BFCP request message
 * @param prim    BFCP Primitive
 * @param attrc   Number of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_reply(struct bfcp_conn *bc, const struct bfcp_msg *req,
	       enum bfcp_prim prim, unsigned attrc, ...)
{
	va_list ap;
	int err;

	if (!bc || !req)
		return EINVAL;

	bc->mb = mem_deref(bc->mb);
	tmr_cancel(&bc->tmr2);

	bc->mb = mbuf_alloc(64);
	if (!bc->mb)
		return ENOMEM;

	va_start(ap, attrc);
	err = bfcp_msg_vencode(bc->mb, req->ver, true, prim, req->confid,
			       req->tid, req->userid, attrc, &ap);
	va_end(ap);

	if (err)
		goto out;

	bc->mb->pos = 0;

	err = bfcp_send(bc, &req->src, bc->mb);
	if (err)
		goto out;

	bc->st.prim   = req->prim;
	bc->st.confid = req->confid;
	bc->st.tid    = req->tid;
	bc->st.userid = req->userid;

	tmr_start(&bc->tmr2, BFCP_T2, tmr_handler, bc);

 out:
	if (err)
		bc->mb = mem_deref(bc->mb);

	return err;
}


/**
 * Send a BFCP error response with details
 *
 * @param bc      BFCP connection
 * @param req     BFCP request message
 * @param code    Error code
 * @param details Error details
 * @param len     Details length
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_edreply(struct bfcp_conn *bc, const struct bfcp_msg *req,
		 enum bfcp_err code, const uint8_t *details, size_t len)
{
	struct bfcp_errcode errcode;

	errcode.code    = code;
	errcode.details = (uint8_t *)details;
	errcode.len     = len;

	return bfcp_reply(bc, req, BFCP_ERROR, 1,
			  BFCP_ERROR_CODE, 0, &errcode);
}


/**
 * Send a BFCP error response
 *
 * @param bc      BFCP connection
 * @param req     BFCP request message
 * @param code    Error code
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_ereply(struct bfcp_conn *bc, const struct bfcp_msg *req,
		enum bfcp_err code)
{
	return bfcp_edreply(bc, req, code, NULL, 0);
}
