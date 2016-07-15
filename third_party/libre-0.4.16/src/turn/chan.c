/**
 * @file chan.c  TURN Channels handling
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_tmr.h>
#include <re_sa.h>
#include <re_md5.h>
#include <re_udp.h>
#include <re_stun.h>
#include <re_turn.h>
#include "turnc.h"


enum {
	CHAN_LIFETIME = 600,
	CHAN_REFRESH = 250,
	CHAN_NUMB_MIN = 0x4000,
	CHAN_NUMB_MAX = 0x7fff
};


struct channels {
	struct hash *ht_numb;
	struct hash *ht_peer;
	uint16_t nr;
};


struct chan {
	struct le he_numb;
	struct le he_peer;
	struct loop_state ls;
	uint16_t nr;
	struct sa peer;
	struct tmr tmr;
	struct turnc *turnc;
	struct stun_ctrans *ct;
	turnc_chan_h *ch;
	void *arg;
};


static int chanbind_request(struct chan *chan, bool reset_ls);


static void channels_destructor(void *data)
{
	struct channels *c = data;

	/* flush from primary hash */
	hash_flush(c->ht_numb);

	mem_deref(c->ht_numb);
	mem_deref(c->ht_peer);
}


static void chan_destructor(void *data)
{
	struct chan *chan = data;

	tmr_cancel(&chan->tmr);
	mem_deref(chan->ct);
	hash_unlink(&chan->he_numb);
	hash_unlink(&chan->he_peer);
}


static bool numb_hash_cmp_handler(struct le *le, void *arg)
{
	const struct chan *chan = le->data;
	const uint16_t *nr = arg;

	return chan->nr == *nr;
}


static bool peer_hash_cmp_handler(struct le *le, void *arg)
{
	const struct chan *chan = le->data;

	return sa_cmp(&chan->peer, arg, SA_ALL);
}


static void timeout(void *arg)
{
	struct chan *chan = arg;
	int err;

	err = chanbind_request(chan, true);
	if (err)
		chan->turnc->th(err, 0, NULL, NULL, NULL, NULL,
				chan->turnc->arg);
}


static void chanbind_resp_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct chan *chan = arg;

	if (err || turnc_request_loops(&chan->ls, scode))
		goto out;

	switch (scode) {

	case 0:
		tmr_start(&chan->tmr, CHAN_REFRESH * 1000, timeout, chan);
		if (chan->ch) {
			chan->ch(chan->arg);
			chan->ch  = NULL;
			chan->arg = NULL;
		}
		return;

	case 401:
	case 438:
		err = turnc_keygen(chan->turnc, msg);
		if (err)
			break;

		err = chanbind_request(chan, false);
		if (err)
			break;

		return;

	default:
		break;
	}

 out:
	chan->turnc->th(err, scode, reason, NULL, NULL, msg, chan->turnc->arg);
}


static int chanbind_request(struct chan *chan, bool reset_ls)
{
	struct turnc *t = chan->turnc;

	if (reset_ls)
		turnc_loopstate_reset(&chan->ls);

	return stun_request(&chan->ct, t->stun, t->proto, t->sock, &t->srv, 0,
			    STUN_METHOD_CHANBIND,
			    t->realm ? t->md5_hash : NULL, sizeof(t->md5_hash),
			    false, chanbind_resp_handler, chan, 6,
			    STUN_ATTR_CHANNEL_NUMBER, &chan->nr,
			    STUN_ATTR_XOR_PEER_ADDR, &chan->peer,
			    STUN_ATTR_USERNAME, t->realm ? t->username : NULL,
			    STUN_ATTR_REALM, t->realm,
			    STUN_ATTR_NONCE, t->nonce,
			    STUN_ATTR_SOFTWARE, stun_software);
}


/**
 * Add a TURN Channel for a peer
 *
 * @param turnc TURN Client
 * @param peer  Peer IP-address
 * @param ch    Channel handler
 * @param arg   Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int turnc_add_chan(struct turnc *turnc, const struct sa *peer,
		   turnc_chan_h *ch, void *arg)
{
	struct chan *chan;
	int err;

	if (!turnc || !peer)
		return EINVAL;

	if (turnc->chans->nr >= CHAN_NUMB_MAX)
		return ERANGE;

	if (turnc_chan_find_peer(turnc, peer))
		return 0;

	chan = mem_zalloc(sizeof(*chan), chan_destructor);
	if (!chan)
		return ENOMEM;

	chan->nr = turnc->chans->nr++;
	chan->peer = *peer;

	hash_append(turnc->chans->ht_numb, chan->nr, &chan->he_numb, chan);
	hash_append(turnc->chans->ht_peer, sa_hash(peer, SA_ALL),
		    &chan->he_peer, chan);

	tmr_init(&chan->tmr);
	chan->turnc = turnc;
	chan->ch = ch;
	chan->arg = arg;

	err = chanbind_request(chan, true);
	if (err)
		mem_deref(chan);

	return err;
}


int turnc_chan_hash_alloc(struct channels **cp, uint32_t bsize)
{
	struct channels *c;
	int err;

	if (!cp)
		return EINVAL;

	c = mem_zalloc(sizeof(*c), channels_destructor);
	if (!c)
		return ENOMEM;

	err = hash_alloc(&c->ht_numb, bsize);
	if (err)
		goto out;

	err = hash_alloc(&c->ht_peer, bsize);
	if (err)
		goto out;

	c->nr = CHAN_NUMB_MIN;

 out:
	if (err)
		mem_deref(c);
	else
		*cp = c;

	return err;
}


struct chan *turnc_chan_find_numb(const struct turnc *turnc, uint16_t nr)
{
	if (!turnc)
		return NULL;

	return list_ledata(hash_lookup(turnc->chans->ht_numb, nr,
				       numb_hash_cmp_handler, &nr));
}


struct chan *turnc_chan_find_peer(const struct turnc *turnc,
				  const struct sa *peer)
{
	if (!turnc)
		return NULL;

	return list_ledata(hash_lookup(turnc->chans->ht_peer,
				       sa_hash(peer, SA_ALL),
				       peer_hash_cmp_handler, (void *)peer));
}


uint16_t turnc_chan_numb(const struct chan *chan)
{
	return chan ? chan->nr : 0;
}


const struct sa *turnc_chan_peer(const struct chan *chan)
{
	return chan ? &chan->peer : NULL;
}


int turnc_chan_hdr_encode(const struct chan_hdr *hdr, struct mbuf *mb)
{
	int err;

	if (!hdr || !mb)
		return EINVAL;

	err  = mbuf_write_u16(mb, htons(hdr->nr));
	err |= mbuf_write_u16(mb, htons(hdr->len));

	return err;
}


int turnc_chan_hdr_decode(struct chan_hdr *hdr, struct mbuf *mb)
{
	if (!hdr || !mb)
		return EINVAL;

	if (mbuf_get_left(mb) < sizeof(*hdr))
		return ENOENT;

	hdr->nr  = ntohs(mbuf_read_u16(mb));
	hdr->len = ntohs(mbuf_read_u16(mb));

	return 0;
}
