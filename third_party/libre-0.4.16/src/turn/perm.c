/**
 * @file perm.c  TURN permission handling
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
	PERM_LIFETIME = 300,
	PERM_REFRESH = 250,
};


struct perm {
	struct le he;
	struct loop_state ls;
	struct sa peer;
	struct tmr tmr;
	struct turnc *turnc;
	struct stun_ctrans *ct;
	turnc_perm_h *ph;
	void *arg;
};


static int createperm_request(struct perm *perm, bool reset_ls);


static void destructor(void *arg)
{
	struct perm *perm = arg;

	tmr_cancel(&perm->tmr);
	mem_deref(perm->ct);
	hash_unlink(&perm->he);
}


static bool hash_cmp_handler(struct le *le, void *arg)
{
	const struct perm *perm = le->data;

	return sa_cmp(&perm->peer, arg, SA_ADDR);
}


static struct perm *perm_find(const struct turnc *turnc, const struct sa *peer)
{
	return list_ledata(hash_lookup(turnc->perms, sa_hash(peer, SA_ADDR),
				       hash_cmp_handler, (void *)peer));
}


static void timeout(void *arg)
{
	struct perm *perm = arg;
	int err;

	err = createperm_request(perm, true);
	if (err)
		perm->turnc->th(err, 0, NULL, NULL, NULL, NULL,
				perm->turnc->arg);
}


static void createperm_resp_handler(int err, uint16_t scode,
				    const char *reason,
				    const struct stun_msg *msg, void *arg)
{
	struct perm *perm = arg;

	if (err || turnc_request_loops(&perm->ls, scode))
		goto out;

	switch (scode) {

	case 0:
		tmr_start(&perm->tmr, PERM_REFRESH * 1000, timeout, perm);
		if (perm->ph) {
			perm->ph(perm->arg);
			perm->ph  = NULL;
			perm->arg = NULL;
		}
		return;

	case 401:
	case 438:
		err = turnc_keygen(perm->turnc, msg);
		if (err)
			break;

		err = createperm_request(perm, false);
		if (err)
			break;

		return;

	default:
		break;
	}

 out:
	perm->turnc->th(err, scode, reason, NULL, NULL, msg, perm->turnc->arg);
}


static int createperm_request(struct perm *perm, bool reset_ls)
{
	struct turnc *t = perm->turnc;

	if (reset_ls)
		turnc_loopstate_reset(&perm->ls);

	return stun_request(&perm->ct, t->stun, t->proto, t->sock, &t->srv, 0,
			    STUN_METHOD_CREATEPERM,
			    t->realm ? t->md5_hash : NULL, sizeof(t->md5_hash),
			    false, createperm_resp_handler, perm, 5,
			    STUN_ATTR_XOR_PEER_ADDR, &perm->peer,
			    STUN_ATTR_USERNAME, t->realm ? t->username : NULL,
			    STUN_ATTR_REALM, t->realm,
			    STUN_ATTR_NONCE, t->nonce,
			    STUN_ATTR_SOFTWARE, stun_software);
}


/**
 * Add TURN Permission for a peer
 *
 * @param turnc TURN Client
 * @param peer  Peer IP-address
 * @param ph    Permission handler
 * @param arg   Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int turnc_add_perm(struct turnc *turnc, const struct sa *peer,
		   turnc_perm_h *ph, void *arg)
{
	struct perm *perm;
	int err;

	if (!turnc || !peer)
		return EINVAL;

	if (perm_find(turnc, peer))
		return 0;

	perm = mem_zalloc(sizeof(*perm), destructor);
	if (!perm)
		return ENOMEM;

	hash_append(turnc->perms, sa_hash(peer, SA_ADDR), &perm->he, perm);
	tmr_init(&perm->tmr);
	perm->peer = *peer;
	perm->turnc = turnc;
	perm->ph = ph;
	perm->arg = arg;

	err = createperm_request(perm, true);
	if (err)
		mem_deref(perm);

	return err;
}


int turnc_perm_hash_alloc(struct hash **ht, uint32_t bsize)
{
	return hash_alloc(ht, bsize);
}
