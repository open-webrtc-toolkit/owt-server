/**
 * @file comp.c  ICE Media component
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sys.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_stun.h>
#include <re_turn.h>
#include <re_ice.h>
#include "ice.h"


#define DEBUG_MODULE "icecomp"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


enum {COMPID_MIN = 1, COMPID_MAX = 255};


static bool helper_recv_handler(struct sa *src, struct mbuf *mb, void *arg)
{
	struct icem_comp *comp = arg;
	struct icem *icem = comp->icem;
	struct stun_msg *msg = NULL;
	struct stun_unknown_attr ua;
	const size_t start = mb->pos;

#if 0
	re_printf("{%d} UDP recv_helper: %u bytes from %J\n",
		  comp->id, mbuf_get_left(mb), src);
#endif

	if (stun_msg_decode(&msg, mb, &ua))
		return false;

	if (STUN_METHOD_BINDING == stun_msg_method(msg)) {

		switch (stun_msg_class(msg)) {

		case STUN_CLASS_REQUEST:
			(void)icem_stund_recv(comp, src, msg, start);
			break;

		default:
			(void)stun_ctrans_recv(icem->ice->stun, msg, &ua);
			break;
		}
	}

	mem_deref(msg);

	return true;  /* handled */
}


static void destructor(void *arg)
{
	struct icem_comp *comp = arg;

	tmr_cancel(&comp->tmr_ka);
	mem_deref(comp->ct_gath);
	mem_deref(comp->turnc);
	mem_deref(comp->cp_sel);
	mem_deref(comp->def_lcand);
	mem_deref(comp->def_rcand);
	mem_deref(comp->uh);
	mem_deref(comp->sock);
}


static struct ice_cand *cand_default(const struct list *lcandl,
				     unsigned compid)
{
	struct ice_cand *def = NULL;
	struct le *le;

	/* NOTE: list must be sorted by priority */
	for (le = list_head(lcandl); le; le = le->next) {

		struct ice_cand *cand = le->data;

		if (cand->compid != compid)
			continue;

		switch (cand->type) {

		case ICE_CAND_TYPE_RELAY:
			return cand;

		case ICE_CAND_TYPE_SRFLX:
			if (!def || ICE_CAND_TYPE_SRFLX != def->type)
				def = cand;
			break;

		case ICE_CAND_TYPE_HOST:
			if (!def)
				def = cand;
			break;

		default:
			break;
		}
	}

	return def;
}


int icem_comp_alloc(struct icem_comp **cp, struct icem *icem, int id,
		    void *sock)
{
	struct icem_comp *comp;
	struct sa local;
	int err;

	if (!cp || !icem || id<1 || id>255 || !sock)
		return EINVAL;

	comp = mem_zalloc(sizeof(*comp), destructor);
	if (!comp)
		return ENOMEM;

	comp->id = id;
	comp->sock = mem_ref(sock);
	comp->icem = icem;

	err = udp_register_helper(&comp->uh, sock, icem->layer,
				  NULL, helper_recv_handler, comp);
	if (err)
		goto out;

	err = udp_local_get(comp->sock, &local);
	if (err)
		goto out;

	comp->lport = sa_port(&local);

 out:
	if (err)
		mem_deref(comp);
	else
		*cp = comp;

	return err;
}


int icem_comp_set_default_cand(struct icem_comp *comp)
{
	struct ice_cand *cand;

	if (!comp)
		return EINVAL;

	cand = cand_default(&comp->icem->lcandl, comp->id);
	if (!cand)
		return ENOENT;

	mem_deref(comp->def_lcand);
	comp->def_lcand = mem_ref(cand);

	return 0;
}


void icem_comp_set_default_rcand(struct icem_comp *comp,
				 struct ice_cand *rcand)
{
	if (!comp)
		return;

	icecomp_printf(comp, "Set default remote candidate: %s:%J\n",
		       ice_cand_type2name(rcand->type), &rcand->addr);

	mem_deref(comp->def_rcand);
	comp->def_rcand = mem_ref(rcand);

	if (comp->turnc) {
		icecomp_printf(comp, "Add TURN Channel to peer %J\n",
			       &rcand->addr);

		(void)turnc_add_chan(comp->turnc, &rcand->addr, NULL, NULL);
	}
}


void icem_comp_set_selected(struct icem_comp *comp, struct ice_candpair *cp)
{
	if (!comp || !cp)
		return;

	if (cp->state != ICE_CANDPAIR_SUCCEEDED) {
		DEBUG_WARNING("{%s.%u} set_selected: invalid state %s\n",
			      comp->icem->name, comp->id,
			      ice_candpair_state2name(cp->state));
	}

	mem_deref(comp->cp_sel);
	comp->cp_sel = mem_ref(cp);
}


struct icem_comp *icem_comp_find(const struct icem *icem, unsigned compid)
{
	struct le *le;

	if (!icem)
		return NULL;

	for (le = icem->compl.head; le; le = le->next) {

		struct icem_comp *comp = le->data;

		if (comp->id == compid)
			return comp;
	}

	return NULL;
}


static void timeout(void *arg)
{
	struct icem_comp *comp = arg;
	struct ice_candpair *cp;

	tmr_start(&comp->tmr_ka, ICE_DEFAULT_Tr * 1000 + rand_u16() % 1000,
		  timeout, comp);

	/* find selected candidate-pair */
	cp = comp->cp_sel;
	if (!cp)
		return;

	(void)stun_indication(comp->icem->proto, comp->sock, &cp->rcand->addr,
			      (cp->lcand->type == ICE_CAND_TYPE_RELAY) ? 4 : 0,
			      STUN_METHOD_BINDING, NULL, 0, true, 0);
}


void icem_comp_keepalive(struct icem_comp *comp, bool enable)
{
	if (!comp)
		return;

	if (enable) {
		tmr_start(&comp->tmr_ka, ICE_DEFAULT_Tr * 1000, timeout, comp);
	}
	else {
		tmr_cancel(&comp->tmr_ka);
	}
}


void icecomp_printf(struct icem_comp *comp, const char *fmt, ...)
{
	va_list ap;

	if (!comp || !comp->icem->ice->conf.debug)
		return;

	va_start(ap, fmt);
	(void)re_printf("{%11s.%u} %v", comp->icem->name, comp->id, fmt, &ap);
	va_end(ap);
}


int icecomp_debug(struct re_printf *pf, const struct icem_comp *comp)
{
	if (!comp)
		return 0;

	return re_hprintf(pf, "id=%u ldef=%J rdef=%J concluded=%d",
			  comp->id,
			  comp->def_lcand ? &comp->def_lcand->addr : NULL,
			  comp->def_rcand ? &comp->def_rcand->addr : NULL,
			  comp->concluded);
}
