/**
 * @file bfcp/conn.c BFCP Connection
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
#include <re_udp.h>
#include <re_tmr.h>
#include <re_bfcp.h>
#include "bfcp.h"


static void destructor(void *arg)
{
	struct bfcp_conn *bc = arg;

	list_flush(&bc->ctransl);
	tmr_cancel(&bc->tmr1);
	tmr_cancel(&bc->tmr2);
	mem_deref(bc->us);
	mem_deref(bc->mb);
}


static bool strans_cmp(const struct bfcp_strans *st,
		       const struct bfcp_msg *msg)
{
	if (st->tid != msg->tid)
		return false;

	if (st->prim != msg->prim)
		return false;

	if (st->confid != msg->confid)
		return false;

	if (st->userid != msg->userid)
		return false;

	return true;
}


static void udp_recv_handler(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct bfcp_conn *bc = arg;
	struct bfcp_msg *msg;
	int err;

	err = bfcp_msg_decode(&msg, mb);
	if (err)
		return;

	msg->src = *src;

	if (bfcp_handle_response(bc, msg))
		goto out;

	if (bc->mb && strans_cmp(&bc->st, msg)) {
		(void)bfcp_send(bc, &msg->src, bc->mb);
		goto out;
	}

	if (bc->recvh)
		bc->recvh(msg, bc->arg);

out:
	mem_deref(msg);
}


/**
 * Create BFCP connection
 *
 * @param bcp   Pointer to BFCP connection
 * @param tp    BFCP Transport type
 * @param laddr Optional listening address/port
 * @param tls   TLS Context (optional)
 * @param recvh Receive handler
 * @param arg   Receive handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_listen(struct bfcp_conn **bcp, enum bfcp_transp tp, struct sa *laddr,
		struct tls *tls, bfcp_recv_h *recvh, void *arg)
{
	struct bfcp_conn *bc;
	int err;
	(void)tls;

	if (!bcp)
		return EINVAL;

	bc = mem_zalloc(sizeof(*bc), destructor);
	if (!bc)
		return ENOMEM;

	bc->tp    = tp;
	bc->recvh = recvh;
	bc->arg   = arg;

	switch (bc->tp) {

	case BFCP_UDP:
		err = udp_listen(&bc->us, laddr, udp_recv_handler, bc);
		if (err)
			goto out;

		if (laddr) {
			err = udp_local_get(bc->us, laddr);
			if (err)
				goto out;
		}
		break;

	default:
		err = ENOSYS;
		goto out;
	}

 out:
	if (err)
		mem_deref(bc);
	else
		*bcp = bc;

	return err;
}


int bfcp_send(struct bfcp_conn *bc, const struct sa *dst, struct mbuf *mb)
{
	if (!bc || !dst || !mb)
		return EINVAL;

	switch (bc->tp) {

	case BFCP_UDP:
		return udp_send(bc->us, dst, mb);

	default:
		return ENOSYS;
	}
}


void *bfcp_sock(const struct bfcp_conn *bc)
{
	return bc ? bc->us : NULL;
}
