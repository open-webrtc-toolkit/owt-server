/**
 * @file stun/ctrans.c  STUN Client transactions
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_tcp.h>
#include <re_srtp.h>
#include <re_tls.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_md5.h>
#include <re_stun.h>
#include "stun.h"


struct stun_ctrans {
	struct le le;
	struct tmr tmr;
	struct sa dst;
	uint8_t tid[STUN_TID_SIZE];
	struct stun_ctrans **ctp;
	uint8_t *key;
	size_t keylen;
	void *sock;
	struct mbuf *mb;
	size_t pos;
	struct stun *stun;
	stun_resp_h *resph;
	void *arg;
	int proto;
	uint32_t txc;
	uint32_t ival;
	uint16_t met;
};


static void completed(struct stun_ctrans *ct, int err, uint16_t scode,
		      const char *reason, const struct stun_msg *msg)
{
	stun_resp_h *resph = ct->resph;
	void *arg = ct->arg;

	list_unlink(&ct->le);
	tmr_cancel(&ct->tmr);

	if (ct->ctp) {
		*ct->ctp = NULL;
		ct->ctp = NULL;
	}

	ct->resph = NULL;

	/* must be destroyed before calling handler */
	mem_deref(ct);

	if (resph)
		resph(err, scode, reason, msg, arg);
}


static void destructor(void *arg)
{
	struct stun_ctrans *ct = arg;

	list_unlink(&ct->le);
	tmr_cancel(&ct->tmr);
	mem_deref(ct->key);
	mem_deref(ct->sock);
	mem_deref(ct->mb);
}


static void timeout_handler(void *arg)
{
	struct stun_ctrans *ct = arg;
	const struct stun_conf *cfg = stun_conf(ct->stun);
	int err = ETIMEDOUT;

	if (ct->txc++ >= cfg->rc)
		goto error;

	ct->mb->pos = ct->pos;

	err = stun_send(ct->proto, ct->sock, &ct->dst, ct->mb);
	if (err)
		goto error;

	ct->ival = (ct->txc >= cfg->rc) ? cfg->rto * cfg->rm : ct->ival * 2;

	tmr_start(&ct->tmr, ct->ival, timeout_handler, ct);
	return;

 error:
	completed(ct, err, 0, NULL, NULL);
}


static bool match_handler(struct le *le, void *arg)
{
	struct stun_ctrans *ct = le->data;
	struct stun_msg *msg = arg;

	if (ct->met != stun_msg_method(msg))
		return false;

	if (memcmp(ct->tid, stun_msg_tid(msg), STUN_TID_SIZE))
		return false;

	return true;
}


static void udp_recv_handler(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct stun *stun = arg;
	(void)src;

	(void)stun_recv(stun, mb);
}


static void tcp_recv_handler(struct mbuf *mb, void *arg)
{
	struct stun_ctrans *ct = arg;

	(void)stun_recv(ct->stun, mb);
}


static void tcp_estab_handler(void *arg)
{
	struct stun_ctrans *ct = arg;
	int err;

	err = tcp_send(ct->sock, ct->mb);
	if (!err)
		return;

	completed(ct, err, 0, NULL, NULL);
}


static void tcp_close_handler(int err, void *arg)
{
	struct stun_ctrans *ct = arg;

	completed(ct, err, 0, NULL, NULL);
}


/**
 * Handle an incoming STUN message to a Client Transaction
 *
 * @param stun STUN instance
 * @param msg  STUN message
 * @param ua   Unknown attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_ctrans_recv(struct stun *stun, const struct stun_msg *msg,
		     const struct stun_unknown_attr *ua)
{
	struct stun_errcode ec = {0, "OK"};
	struct stun_attr *errcode;
	struct stun_ctrans *ct;
	int err = 0, herr = 0;

	if (!stun || !msg || !ua)
		return EINVAL;

	switch (stun_msg_class(msg)) {

	case STUN_CLASS_ERROR_RESP:
		errcode = stun_msg_attr(msg, STUN_ATTR_ERR_CODE);
		if (!errcode)
			herr = EPROTO;
		else
			ec = errcode->v.err_code;
		/*@fallthrough@*/

	case STUN_CLASS_SUCCESS_RESP:
		ct = list_ledata(list_apply(&stun->ctl, true,
					    match_handler, (void *)msg));
		if (!ct) {
			err = ENOENT;
			break;
		}

		switch (ec.code) {

		case 401:
		case 438:
			break;

		default:
			if (!ct->key)
				break;

			err = stun_msg_chk_mi(msg, ct->key, ct->keylen);
			break;
		}

		if (err)
			break;

		if (!herr && ua->typec > 0)
			herr = EPROTO;

		completed(ct, herr, ec.code, ec.reason, msg);
		break;

	default:
		break;
	}

	return err;
}


int stun_ctrans_request(struct stun_ctrans **ctp, struct stun *stun, int proto,
			void *sock, const struct sa *dst, struct mbuf *mb,
			const uint8_t tid[], uint16_t met, const uint8_t *key,
			size_t keylen, stun_resp_h *resph, void *arg)
{
	struct stun_ctrans *ct;
	int err = 0;

	if (!stun || !mb)
		return EINVAL;

	ct = mem_zalloc(sizeof(*ct), destructor);
	if (!ct)
		return ENOMEM;

	list_append(&stun->ctl, &ct->le, ct);
	memcpy(ct->tid, tid, STUN_TID_SIZE);
	ct->proto = proto;
	ct->sock  = mem_ref(sock);
	ct->mb    = mem_ref(mb);
	ct->pos   = mb->pos;
	ct->stun  = stun;
	ct->met   = met;

	if (key) {
		ct->key = mem_alloc(keylen, NULL);
		if (!ct->key) {
			err = ENOMEM;
			goto out;
		}

		memcpy(ct->key, key, keylen);
		ct->keylen = keylen;
	}

	switch (proto) {

	case IPPROTO_UDP:
		if (!dst) {
			err = EINVAL;
			break;
		}

		ct->dst = *dst;
		ct->ival = stun_conf(stun)->rto;
		tmr_start(&ct->tmr, ct->ival, timeout_handler, ct);

		if (!sock) {
			err = udp_listen((struct udp_sock **)&ct->sock, NULL,
					 udp_recv_handler, stun);
			if (err)
				break;
		}

		ct->txc = 1;
		err = udp_send(ct->sock, dst, mb);
		break;

	case IPPROTO_TCP:
		ct->txc = stun_conf(stun)->rc;
		tmr_start(&ct->tmr, stun_conf(stun)->ti, timeout_handler, ct);
		if (sock) {
			err = tcp_send(sock, mb);
			break;
		}

		err = tcp_connect((struct tcp_conn **)&ct->sock, dst,
				  tcp_estab_handler, tcp_recv_handler,
				  tcp_close_handler, ct);
		break;

#ifdef USE_DTLS
	case STUN_TRANSP_DTLS:
		if (!sock) {
			err = EINVAL;
			break;
		}

		ct->ival = stun_conf(stun)->rto;
		tmr_start(&ct->tmr, ct->ival, timeout_handler, ct);

		ct->txc = 1;
		err = dtls_send(ct->sock, mb);
		break;
#endif

	default:
		err = EPROTONOSUPPORT;
		break;
	}

 out:
	if (!err) {
		if (ctp) {
			ct->ctp = ctp;
			*ctp = ct;
		}

		ct->resph = resph;
		ct->arg   = arg;
	}
	else
		mem_deref(ct);

	return err;
}


static bool close_handler(struct le *le, void *arg)
{
	struct stun_ctrans *ct = le->data;
	(void)arg;

	completed(ct, ECONNABORTED, 0, NULL, NULL);

	return false;
}


void stun_ctrans_close(struct stun *stun)
{
	if (!stun)
		return;

	(void)list_apply(&stun->ctl, true, close_handler, NULL);
}


static bool debug_handler(struct le *le, void *arg)
{
	struct stun_ctrans *ct = le->data;
	struct re_printf *pf = arg;
	int err = 0;

	err |= re_hprintf(pf, " method=%s", stun_method_name(ct->met));
	err |= re_hprintf(pf, " tid=%w", ct->tid, sizeof(ct->tid));
	err |= re_hprintf(pf, " rto=%ums", stun_conf(ct->stun)->rto);
	err |= re_hprintf(pf, " tmr=%llu", tmr_get_expire(&ct->tmr));
	err |= re_hprintf(pf, " n=%u", ct->txc);
	err |= re_hprintf(pf, " interval=%u", ct->ival);
	err |= re_hprintf(pf, "\n");

	return 0 != err;
}


int stun_ctrans_debug(struct re_printf *pf, const struct stun *stun)
{
	int err;

	if (!stun)
		return 0;

	err = re_hprintf(pf, "STUN client transactions: (%u)\n",
			 list_count(&stun->ctl));

	(void)list_apply(&stun->ctl, true, debug_handler, pf);

	return err;
}
