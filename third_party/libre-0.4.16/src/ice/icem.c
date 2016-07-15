/**
 * @file icem.c  ICE Media stream
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sa.h>
#include <re_stun.h>
#include <re_turn.h>
#include <re_ice.h>
#include "ice.h"


#define DEBUG_MODULE "icem"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void icem_destructor(void *data)
{
	struct icem *icem = data;

	list_unlink(&icem->le);
	tmr_cancel(&icem->tmr_pace);
	list_flush(&icem->compl);
	list_flush(&icem->validl);
	list_flush(&icem->checkl);
	list_flush(&icem->lcandl);
	list_flush(&icem->rcandl);
	mem_deref(icem->rufrag);
	mem_deref(icem->rpwd);
}


/**
 * Add a new ICE Media object to the ICE Session
 *
 * @param icemp  Pointer to allocated ICE Media object
 * @param ice    ICE Session
 * @param proto  Transport protocol
 * @param layer  Protocol stack layer
 * @param gh     Gather handler
 * @param chkh   Connectivity check handler
 * @param arg    Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_alloc(struct icem **icemp, struct ice *ice, int proto, int layer,
	       ice_gather_h *gh, ice_connchk_h *chkh, void *arg)
{
	struct icem *icem;
	int err = 0;

	if (!ice)
		return EINVAL;

	if (proto != IPPROTO_UDP)
		return EPROTONOSUPPORT;

	icem = mem_zalloc(sizeof(*icem), icem_destructor);
	if (!icem)
		return ENOMEM;

	tmr_init(&icem->tmr_pace);
	list_init(&icem->lcandl);
	list_init(&icem->rcandl);
	list_init(&icem->checkl);
	list_init(&icem->validl);

	icem->ice   = ice;
	icem->layer = layer;
	icem->proto = proto;
	icem->state = ICE_CHECKLIST_NULL;
	icem->nstun = 0;
	icem->gh    = gh;
	icem->chkh  = chkh;
	icem->arg   = arg;

	if (err)
		goto out;

	list_append(&ice->ml, &icem->le, icem);

 out:
	if (err)
		mem_deref(icem);
	else if (icemp)
		*icemp = icem;

	return err;
}


/**
 * Set the name of the ICE Media object, used for debugging
 *
 * @param icem  ICE Media object
 * @param name  Media name
 */
void icem_set_name(struct icem *icem, const char *name)
{
	if (!icem)
		return;

	str_ncpy(icem->name, name, sizeof(icem->name));
}


/**
 * Add a new component to the ICE Media object
 *
 * @param icem    ICE Media object
 * @param compid  Component ID
 * @param sock    Application protocol socket
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_comp_add(struct icem *icem, unsigned compid, void *sock)
{
	struct icem_comp *comp;
	int err;

	if (!icem)
		return EINVAL;

	if (icem_comp_find(icem, compid))
		return EALREADY;

	err = icem_comp_alloc(&comp, icem, compid, sock);
	if (err)
		return err;

	list_append(&icem->compl, &comp->le, comp);

	return 0;
}


/**
 * Add a new candidate to the ICE Media object
 *
 * @param icem    ICE Media object
 * @param compid  Component ID
 * @param lprio   Local priority
 * @param ifname  Name of the network interface
 * @param addr    Local network address
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_cand_add(struct icem *icem, unsigned compid, uint16_t lprio,
		  const char *ifname, const struct sa *addr)
{
	if (!icem_comp_find(icem, compid))
		return ENOENT;

	return icem_lcand_add_base(icem, compid, lprio, ifname,
				   ICE_TRANSP_UDP, addr);
}


static void *unique_handler(struct le *le1, struct le *le2)
{
	struct ice_cand *c1 = le1->data, *c2 = le2->data;

	if (c1->base != c2->base || !sa_cmp(&c1->addr, &c2->addr, SA_ALL))
		return NULL;

	/* remove candidate with lower priority */
	return c1->prio < c2->prio ? c1 : c2;
}


/** Eliminating Redundant Candidates */
void icem_cand_redund_elim(struct icem *icem)
{
	uint32_t n;

	n = ice_list_unique(&icem->lcandl, unique_handler);
	if (n > 0) {
		icem_printf(icem, "redundant candidates eliminated: %u\n", n);
	}
}


/**
 * Get the Default Local Candidate
 *
 * @param icem   ICE Media object
 * @param compid Component ID
 *
 * @return Default Local Candidate address if set, otherwise NULL
 */
const struct sa *icem_cand_default(struct icem *icem, unsigned compid)
{
	const struct icem_comp *comp = icem_comp_find(icem, compid);

	if (!comp || !comp->def_lcand)
		return NULL;

	return &comp->def_lcand->addr;
}


/**
 * Verifying ICE Support and set default remote candidate
 *
 * @param icem   ICE Media
 * @param compid Component ID
 * @param raddr  Address of default remote candidate
 *
 * @return True if ICE is supported, otherwise false
 */
bool icem_verify_support(struct icem *icem, unsigned compid,
			 const struct sa *raddr)
{
	struct ice_cand *rcand;
	bool match;

	if (!icem)
		return false;

	rcand = icem_cand_find(&icem->rcandl, compid, raddr);
	match = rcand != NULL;

	if (!match)
		icem->mismatch = true;

	if (rcand) {
		icem_comp_set_default_rcand(icem_comp_find(icem, compid),
					    rcand);
	}

	return match;
}


/**
 * Add a TURN Channel for the selected remote address
 *
 * @param icem   ICE Media object
 * @param compid Component ID
 * @param raddr  Remote network address
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_add_chan(struct icem *icem, unsigned compid, const struct sa *raddr)
{
	struct icem_comp *comp;

	if (!icem)
		return EINVAL;

	comp = icem_comp_find(icem, compid);
	if (!comp)
		return ENOENT;

	if (comp->turnc) {
		DEBUG_NOTICE("{%s.%u} Add TURN Channel to peer %J\n",
			     comp->icem->name, comp->id, raddr);

		return turnc_add_chan(comp->turnc, raddr, NULL, NULL);
	}

	return 0;
}


static void purge_relayed(struct icem *icem, struct icem_comp *comp)
{
	if (comp->turnc) {
		DEBUG_NOTICE("{%s.%u} purge local RELAY candidates\n",
			     icem->name, comp->id);
	}

	/*
	 * Purge all Candidate-Pairs where the Local candidate
	 * is of type "Relay"
	 */
	icem_candpairs_flush(&icem->checkl, ICE_CAND_TYPE_RELAY, comp->id);
	icem_candpairs_flush(&icem->validl, ICE_CAND_TYPE_RELAY, comp->id);

	comp->turnc = mem_deref(comp->turnc);
}


/**
 * Update the ICE Media object
 *
 * @param icem ICE Media object
 */
void icem_update(struct icem *icem)
{
	struct le *le;

	if (!icem)
		return;

	for (le = icem->compl.head; le; le = le->next) {

		struct icem_comp *comp = le->data;

		/* remove TURN client if not used by local "Selected" */
		if (comp->cp_sel) {

			if (comp->cp_sel->lcand->type != ICE_CAND_TYPE_RELAY)
				purge_relayed(icem, comp);
		}
	}
}


/**
 * Get the ICE Mismatch flag of the ICE Media object
 *
 * @param icem ICE Media object
 *
 * @return True if ICE mismatch, otherwise false
 */
bool icem_mismatch(const struct icem *icem)
{
	return icem ? icem->mismatch : true;
}


/**
 * Print debug information for the ICE Media
 *
 * @param pf   Print function for debug output
 * @param icem ICE Media object
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_debug(struct re_printf *pf, const struct icem *icem)
{
	struct le *le;
	int err = 0;

	if (!icem)
		return 0;

	err |= re_hprintf(pf, "----- ICE Media <%s> -----\n", icem->name);

	err |= re_hprintf(pf, " Components: (%u)\n", list_count(&icem->compl));
	for (le = icem->compl.head; le; le = le->next) {
		struct icem_comp *comp = le->data;

		err |= re_hprintf(pf, "  %H\n", icecomp_debug, comp);
	}

	err |= re_hprintf(pf, " Local Candidates: %H",
			  icem_cands_debug, &icem->lcandl);
	err |= re_hprintf(pf, " Remote Candidates: %H",
			  icem_cands_debug, &icem->rcandl);
	err |= re_hprintf(pf, " Check list: [state=%s]%H",
			  ice_checkl_state2name(icem->state),
			  icem_candpairs_debug, &icem->checkl);
	err |= re_hprintf(pf, " Valid list: %H",
			  icem_candpairs_debug, &icem->validl);

	return err;
}


/**
 * Get the list of Local Candidates (struct cand)
 *
 * @param icem ICE Media object
 *
 * @return List of Local Candidates
 */
struct list *icem_lcandl(const struct icem *icem)
{
	return icem ? (struct list *)&icem->lcandl : NULL;
}


/**
 * Get the list of Remote Candidates (struct cand)
 *
 * @param icem ICE Media object
 *
 * @return List of Remote Candidates
 */
struct list *icem_rcandl(const struct icem *icem)
{
	return icem ? (struct list *)&icem->rcandl : NULL;
}


/**
 * Get the checklist of Candidate Pairs
 *
 * @param icem ICE Media object
 *
 * @return Checklist (struct ice_candpair)
 */
struct list *icem_checkl(const struct icem *icem)
{
	return icem ? (struct list *)&icem->checkl : NULL;
}


/**
 * Get the list of valid Candidate Pairs
 *
 * @param icem ICE Media object
 *
 * @return Validlist (struct ice_candpair)
 */
struct list *icem_validl(const struct icem *icem)
{
	return icem ? (struct list *)&icem->validl : NULL;
}


/**
 * Set the default local candidates, for ICE-lite mode only
 *
 * @param icem ICE Media object
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_lite_set_default_candidates(struct icem *icem)
{
	struct le *le;
	int err = 0;

	if (icem->ice->lmode != ICE_MODE_LITE)
		return EINVAL;

	for (le = icem->compl.head; le; le = le->next) {

		struct icem_comp *comp = le->data;

		err |= icem_comp_set_default_cand(comp);
	}

	return err;
}


void icem_printf(struct icem *icem, const char *fmt, ...)
{
	va_list ap;

	if (!icem || !icem->ice->conf.debug)
		return;

	va_start(ap, fmt);
	(void)re_printf("{%11s. } %v", icem->name, fmt, &ap);
	va_end(ap);
}
