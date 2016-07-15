/**
 * @file turnc.c  TURN Client implementation
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_md5.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_tmr.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_tcp.h>
#include <re_srtp.h>
#include <re_tls.h>
#include <re_stun.h>
#include <re_turn.h>
#include "turnc.h"


#define DEBUG_MODULE "turnc"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/** TURN Client protocol values */
enum {
	PERM_HASH_SIZE = 16,
	CHAN_HASH_SIZE = 16,
	FAILC_MAX = 16, /**< Maximum number of request errors for loopcheck. */
	STUN_ATTR_ADDR4_SIZE = 8,
	STUN_ATTR_ADDR6_SIZE = 20,
};


static const uint8_t sendind_tid[STUN_TID_SIZE];

static int allocate_request(struct turnc *t);
static int refresh_request(struct turnc *t, uint32_t lifetime, bool reset_ls,
			   stun_resp_h *resph, void *arg);
static void refresh_resp_handler(int err, uint16_t scode, const char *reason,
				 const struct stun_msg *msg, void *arg);


static void destructor(void *arg)
{
	struct turnc *turnc = arg;

	if (turnc->allocated)
		(void)refresh_request(turnc, 0, true, NULL, NULL);

	tmr_cancel(&turnc->tmr);
	mem_deref(turnc->ct);

	hash_flush(turnc->perms);
	mem_deref(turnc->perms);
	mem_deref(turnc->chans);
	mem_deref(turnc->username);
	mem_deref(turnc->password);
	mem_deref(turnc->nonce);
	mem_deref(turnc->realm);
	mem_deref(turnc->stun);
	mem_deref(turnc->uh);
	mem_deref(turnc->sock);
}


static void timeout(void *arg)
{
	struct turnc *turnc = arg;
	int err;

	err = refresh_request(turnc, turnc->lifetime, true,
			      refresh_resp_handler, turnc);
	if (err)
		turnc->th(err, 0, NULL, NULL, NULL, NULL, turnc->arg);
}


static void refresh_timer(struct turnc *turnc)
{
	const uint32_t t = turnc->lifetime*1000*3/4;

	DEBUG_INFO("Start refresh timer.. %u seconds\n", t/1000);

	tmr_start(&turnc->tmr, t, timeout, turnc);
}


static void allocate_resp_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct stun_attr *map = NULL, *rel = NULL, *ltm, *alt;
	struct turnc *turnc = arg;

	if (err || turnc_request_loops(&turnc->ls, scode))
		goto out;

	switch (scode) {

	case 0:
		map = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
		rel = stun_msg_attr(msg, STUN_ATTR_XOR_RELAY_ADDR);
		ltm = stun_msg_attr(msg, STUN_ATTR_LIFETIME);
		if (!rel || !map) {
			DEBUG_WARNING("xor_mapped/relay addr attr missing\n");
			err = EPROTO;
			break;
		}

		if (ltm)
			turnc->lifetime = ltm->v.lifetime;

		turnc->allocated = true;
		refresh_timer(turnc);
		break;

	case 300:
		if (turnc->proto == IPPROTO_TCP ||
		    turnc->proto == STUN_TRANSP_DTLS)
			break;

		alt = stun_msg_attr(msg, STUN_ATTR_ALT_SERVER);
		if (!alt)
			break;

		turnc->psrv = turnc->srv;
		turnc->srv = alt->v.alt_server;

		err = allocate_request(turnc);
		if (err)
			break;

		return;

	case 401:
	case 438:
		err = turnc_keygen(turnc, msg);
		if (err)
			break;

		err = allocate_request(turnc);
		if (err)
			break;

		return;

	default:
		break;
	}

 out:
	turnc->th(err, scode, reason,
		  rel ? &rel->v.xor_relay_addr : NULL,
		  map ? &map->v.xor_mapped_addr : NULL,
		  msg,
		  turnc->arg);
}


static int allocate_request(struct turnc *t)
{
	const int proto = IPPROTO_UDP;

	return stun_request(&t->ct, t->stun, t->proto, t->sock, &t->srv, 0,
			    STUN_METHOD_ALLOCATE,
			    t->realm ? t->md5_hash : NULL, sizeof(t->md5_hash),
			    false, allocate_resp_handler, t, 6,
			    STUN_ATTR_LIFETIME, &t->lifetime,
			    STUN_ATTR_REQ_TRANSPORT, &proto,
			    STUN_ATTR_USERNAME, t->realm ? t->username : NULL,
			    STUN_ATTR_REALM, t->realm,
			    STUN_ATTR_NONCE, t->nonce,
			    STUN_ATTR_SOFTWARE, stun_software);
}


static void refresh_resp_handler(int err, uint16_t scode, const char *reason,
				 const struct stun_msg *msg, void *arg)
{
	struct turnc *turnc = arg;
	struct stun_attr *ltm;

	if (err || turnc_request_loops(&turnc->ls, scode))
		goto out;

	switch (scode) {

	case 0:
		ltm = stun_msg_attr(msg, STUN_ATTR_LIFETIME);
		if (ltm)
			turnc->lifetime = ltm->v.lifetime;
		refresh_timer(turnc);
		return;

	case 401:
	case 438:
		err = turnc_keygen(turnc, msg);
		if (err)
			break;

		err = refresh_request(turnc, turnc->lifetime, false,
				      refresh_resp_handler, turnc);
		if (err)
			break;

		return;

	default:
		break;
	}

 out:
	turnc->th(err, scode, reason, NULL, NULL, msg, turnc->arg);
}


static int refresh_request(struct turnc *t, uint32_t lifetime, bool reset_ls,
			   stun_resp_h *resph, void *arg)
{
	if (!t)
		return EINVAL;

	if (reset_ls)
		turnc_loopstate_reset(&t->ls);

	if (t->ct)
		t->ct = mem_deref(t->ct);

	return stun_request(&t->ct, t->stun, t->proto, t->sock, &t->srv, 0,
			    STUN_METHOD_REFRESH,
			    t->realm ? t->md5_hash : NULL, sizeof(t->md5_hash),
			    false, resph, arg, 5,
			    STUN_ATTR_LIFETIME, &lifetime,
			    STUN_ATTR_USERNAME, t->realm ? t->username : NULL,
			    STUN_ATTR_REALM, t->realm,
			    STUN_ATTR_NONCE, t->nonce,
			    STUN_ATTR_SOFTWARE, stun_software);
}


static inline size_t stun_indlen(const struct sa *sa)
{
	size_t len = STUN_HEADER_SIZE + STUN_ATTR_HEADER_SIZE * 2;

	switch (sa_af(sa)) {

	case AF_INET:
		len += STUN_ATTR_ADDR4_SIZE;
		break;

#ifdef HAVE_INET6
	case AF_INET6:
		len += STUN_ATTR_ADDR6_SIZE;
		break;
#endif
	}

	return len;
}


static bool udp_send_handler(int *err, struct sa *dst, struct mbuf *mb,
			     void *arg)
{
	struct turnc *turnc = arg;
	size_t pos, indlen;
	struct chan *chan;

	if (mb->pos < CHAN_HDR_SIZE)
		return false;

	chan = turnc_chan_find_peer(turnc, dst);
	if (chan) {
		struct chan_hdr hdr;

		hdr.nr  = turnc_chan_numb(chan);
		hdr.len = mbuf_get_left(mb);

		mb->pos -= CHAN_HDR_SIZE;
		*err = turnc_chan_hdr_encode(&hdr, mb);
		mb->pos -= CHAN_HDR_SIZE;

		*dst = turnc->srv;

		return false;
	}

	indlen = stun_indlen(dst);

	if (mb->pos < indlen)
		return false;

	mb->pos -= indlen;
	pos = mb->pos;
	*err = stun_msg_encode(mb, STUN_METHOD_SEND, STUN_CLASS_INDICATION,
			       sendind_tid, NULL, NULL, 0, false, 0x00, 2,
			       STUN_ATTR_XOR_PEER_ADDR, dst,
			       STUN_ATTR_DATA, mb);
	mb->pos = pos;

	*dst = turnc->srv;

	return false;
}


static bool udp_recv_handler(struct sa *src, struct mbuf *mb, void *arg)
{
	struct stun_attr *peer, *data;
	struct stun_unknown_attr ua;
	struct turnc *turnc = arg;
	struct stun_msg *msg;
	bool hdld = true;

	if (!sa_cmp(&turnc->srv, src, SA_ALL) &&
	    !sa_cmp(&turnc->psrv, src, SA_ALL))
		return false;

	if (stun_msg_decode(&msg, mb, &ua)) {

		struct chan_hdr hdr;
		struct chan *chan;

		if (turnc_chan_hdr_decode(&hdr, mb))
			return true;

		if (mbuf_get_left(mb) < hdr.len)
			return true;

		chan = turnc_chan_find_numb(turnc, hdr.nr);
		if (!chan)
			return true;

		*src = *turnc_chan_peer(chan);

		return false;
	}

	switch (stun_msg_class(msg)) {

	case STUN_CLASS_INDICATION:
		if (ua.typec > 0)
			break;

		if (stun_msg_method(msg) != STUN_METHOD_DATA)
			break;

		peer = stun_msg_attr(msg, STUN_ATTR_XOR_PEER_ADDR);
		data = stun_msg_attr(msg, STUN_ATTR_DATA);
		if (!peer || !data)
			break;

		*src = peer->v.xor_peer_addr;

		mb->pos = data->v.data.pos;
		mb->end = data->v.data.end;

		hdld = false;
		break;

	case STUN_CLASS_ERROR_RESP:
	case STUN_CLASS_SUCCESS_RESP:
		(void)stun_ctrans_recv(turnc->stun, msg, &ua);
		break;

	default:
		break;
	}

	mem_deref(msg);

	return hdld;
}


/**
 * Allocate a TURN Client
 *
 * @param turncp    Pointer to allocated TURN Client
 * @param conf      Optional STUN Configuration
 * @param proto     Transport Protocol
 * @param sock      Transport socket
 * @param layer     Transport layer
 * @param srv       TURN Server IP-address
 * @param username  Authentication username
 * @param password  Authentication password
 * @param lifetime  Allocate lifetime in [seconds]
 * @param th        TURN handler
 * @param arg       Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int turnc_alloc(struct turnc **turncp, const struct stun_conf *conf, int proto,
		void *sock, int layer, const struct sa *srv,
		const char *username, const char *password,
		uint32_t lifetime, turnc_h *th, void *arg)
{
	struct turnc *turnc;
	int err;

	if (!turncp || !sock || !srv || !username || !password || !th)
		return EINVAL;

	turnc = mem_zalloc(sizeof(*turnc), destructor);
	if (!turnc)
		return ENOMEM;

	err = stun_alloc(&turnc->stun, conf, NULL, NULL);
	if (err)
		goto out;

	err = str_dup(&turnc->username, username);
	if (err)
		goto out;

	err = str_dup(&turnc->password, password);
	if (err)
		goto out;

	err = turnc_perm_hash_alloc(&turnc->perms, PERM_HASH_SIZE);
	if (err)
		goto out;

	err =  turnc_chan_hash_alloc(&turnc->chans, CHAN_HASH_SIZE);
	if (err)
		goto out;

	tmr_init(&turnc->tmr);
	turnc->proto = proto;
	turnc->sock = mem_ref(sock);
	turnc->psrv = *srv;
	turnc->srv = *srv;
	turnc->lifetime = lifetime;
	turnc->th = th;
	turnc->arg = arg;

	switch (proto) {

	case IPPROTO_UDP:
		err = udp_register_helper(&turnc->uh, sock, layer,
					  udp_send_handler, udp_recv_handler,
					  turnc);
		break;

	default:
		err = 0;
		break;
	}

	if (err)
		goto out;

	err = allocate_request(turnc);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(turnc);
	else
		*turncp = turnc;

	return err;
}


int turnc_send(struct turnc *turnc, const struct sa *dst, struct mbuf *mb)
{
	size_t pos, indlen;
	struct chan *chan;
	int err;

	if (!turnc || !dst || !mb)
		return EINVAL;

	chan = turnc_chan_find_peer(turnc, dst);
	if (chan) {
		struct chan_hdr hdr;

		if (mb->pos < CHAN_HDR_SIZE)
			return EINVAL;

		hdr.nr  = turnc_chan_numb(chan);
		hdr.len = mbuf_get_left(mb);

		mb->pos -= CHAN_HDR_SIZE;
		pos = mb->pos;

		err = turnc_chan_hdr_encode(&hdr, mb);
		if (err)
			return err;

		if (turnc->proto == IPPROTO_TCP) {

			mb->pos = mb->end;

			/* padding */
			while (hdr.len++ & 0x03) {
				err = mbuf_write_u8(mb, 0x00);
				if (err)
					return err;
			}
		}

		mb->pos = pos;
	}
	else {
		indlen = stun_indlen(dst);

		if (mb->pos < indlen)
			return EINVAL;

		mb->pos -= indlen;
		pos = mb->pos;

		err = stun_msg_encode(mb, STUN_METHOD_SEND,
				      STUN_CLASS_INDICATION, sendind_tid,
				      NULL, NULL, 0, false, 0x00, 2,
				      STUN_ATTR_XOR_PEER_ADDR, dst,
				      STUN_ATTR_DATA, mb);
		if (err)
			return err;

		mb->pos = pos;
	}

	switch (turnc->proto) {

	case IPPROTO_UDP:
		err = udp_send(turnc->sock, &turnc->srv, mb);
		break;

	case IPPROTO_TCP:
		err = tcp_send(turnc->sock, mb);
		break;

#ifdef USE_DTLS
	case STUN_TRANSP_DTLS:
		err = dtls_send(turnc->sock, mb);
		break;
#endif

	default:
		err = EPROTONOSUPPORT;
		break;
	}

	return err;
}


int turnc_recv(struct turnc *turnc, struct sa *src, struct mbuf *mb)
{
	struct stun_attr *peer, *data;
	struct stun_unknown_attr ua;
	struct stun_msg *msg;
	int err = 0;

	if (!turnc || !src || !mb)
		return EINVAL;

	if (stun_msg_decode(&msg, mb, &ua)) {

		struct chan_hdr hdr;
		struct chan *chan;

		if (turnc_chan_hdr_decode(&hdr, mb))
			return EBADMSG;

		if (mbuf_get_left(mb) < hdr.len)
			return EBADMSG;

		chan = turnc_chan_find_numb(turnc, hdr.nr);
		if (!chan)
			return EBADMSG;

		*src = *turnc_chan_peer(chan);

		return 0;
	}

	switch (stun_msg_class(msg)) {

	case STUN_CLASS_INDICATION:
		if (ua.typec > 0) {
			err = ENOSYS;
			break;
		}

		if (stun_msg_method(msg) != STUN_METHOD_DATA) {
			err = ENOSYS;
			break;
		}

		peer = stun_msg_attr(msg, STUN_ATTR_XOR_PEER_ADDR);
		data = stun_msg_attr(msg, STUN_ATTR_DATA);
		if (!peer || !data) {
			err = EPROTO;
			break;
		}

		*src = peer->v.xor_peer_addr;

		mb->pos = data->v.data.pos;
		mb->end = data->v.data.end;
		break;

	case STUN_CLASS_ERROR_RESP:
	case STUN_CLASS_SUCCESS_RESP:
		(void)stun_ctrans_recv(turnc->stun, msg, &ua);
		mb->pos = mb->end;
		break;

	default:
		err = ENOSYS;
		break;
	}

	mem_deref(msg);

	return err;
}


bool turnc_request_loops(struct loop_state *ls, uint16_t scode)
{
	bool loop = false;

	switch (scode) {

	case 0:
		ls->failc = 0;
		break;

	default:
		if (ls->last_scode == scode)
			loop = true;
		/*@fallthrough@*/
	case 300:
		if (++ls->failc >= FAILC_MAX)
			loop = true;

		break;
	}

	ls->last_scode = scode;

	return loop;
}


void turnc_loopstate_reset(struct loop_state *ls)
{
	if (!ls)
		return;

	ls->last_scode = 0;
	ls->failc = 0;
}


int turnc_keygen(struct turnc *turnc, const struct stun_msg *msg)
{
	struct stun_attr *realm, *nonce;

	realm = stun_msg_attr(msg, STUN_ATTR_REALM);
	nonce = stun_msg_attr(msg, STUN_ATTR_NONCE);
	if (!realm || !nonce)
		return EPROTO;

	mem_deref(turnc->realm);
	mem_deref(turnc->nonce);
	turnc->realm = mem_ref(realm->v.realm);
	turnc->nonce = mem_ref(nonce->v.nonce);

	return md5_printf(turnc->md5_hash, "%s:%s:%s",
			  turnc->username, turnc->realm, turnc->password);
}
