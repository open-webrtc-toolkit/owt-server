/**
 * @file chklist.c  ICE Checklist
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
#include <re_sa.h>
#include <re_stun.h>
#include <re_ice.h>
#include "ice.h"


#define DEBUG_MODULE "chklist"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * Forming Candidate Pairs
 */
static int candpairs_form(struct icem *icem)
{
	struct le *le;
	int err = 0;

	if (list_isempty(&icem->lcandl))
		return ENOENT;

	if (list_isempty(&icem->rcandl)) {
		DEBUG_WARNING("%s: no remote candidates\n", icem->name);
		return ENOENT;
	}

	for (le = icem->lcandl.head; le; le = le->next) {

		struct ice_cand *lcand = le->data;
		struct le *rle;

		for (rle = icem->rcandl.head; rle; rle = rle->next) {

			struct ice_cand *rcand = rle->data;

			if (lcand->compid != rcand->compid)
				continue;

			if (sa_af(&lcand->addr) != sa_af(&rcand->addr))
				continue;

			err = icem_candpair_alloc(NULL, icem, lcand, rcand);
			if (err)
				return err;
		}
	}

	return err;
}


/* Replace server reflexive candidates by its base */
static const struct sa *cand_srflx_addr(const struct ice_cand *c)
{
	return (ICE_CAND_TYPE_SRFLX == c->type) ? &c->base->addr : &c->addr;
}


/* return: NULL to keep, pointer to remove object */
static void *unique_handler(struct le *le1, struct le *le2)
{
	struct ice_candpair *cp1 = le1->data, *cp2 = le2->data;

	if (cp1->comp->id != cp2->comp->id)
		return NULL;

	if (!sa_cmp(cand_srflx_addr(cp1->lcand),
		    cand_srflx_addr(cp2->lcand), SA_ALL) ||
	    !sa_cmp(&cp1->rcand->addr, &cp2->rcand->addr, SA_ALL))
		return NULL;

	return cp1->pprio < cp2->pprio ? cp1 : cp2;
}


/**
 * Pruning the Pairs
 */
static void candpair_prune(struct icem *icem)
{
	/* The agent MUST prune the list.
	   This is done by removing a pair if its local and remote
	   candidates are identical to the local and remote candidates
	   of a pair higher up on the priority list.

	   NOTE: This logic assumes the list is sorted by priority
	*/

	uint32_t n = ice_list_unique(&icem->checkl, unique_handler);
	if (n > 0) {
		DEBUG_NOTICE("%s: pruned candidate pairs: %u\n",
			     icem->name, n);
	}
}


/**
 * Computing States
 */
static void candpair_set_states(struct icem *icem)
{
	struct le *le, *le2;

	/*
	For all pairs with the same foundation, it sets the state of
	the pair with the lowest component ID to Waiting.  If there is
	more than one such pair, the one with the highest priority is
	used.
	*/

	for (le = icem->checkl.head; le; le = le->next) {

		struct ice_candpair *cp = le->data;

		for (le2 = icem->checkl.head; le2; le2 = le2->next) {

			struct ice_candpair *cp2 = le2->data;

			if (!icem_candpair_cmp_fnd(cp, cp2))
				continue;

			if (cp2->lcand->compid < cp->lcand->compid &&
			    cp2->pprio > cp->pprio)
				cp = cp2;
		}

		icem_candpair_set_state(cp, ICE_CANDPAIR_WAITING);
	}
}


/**
 * Forming the Check Lists
 *
 *   To form the check list for a media stream,
 *   the agent forms candidate pairs, computes a candidate pair priority,
 *   orders the pairs by priority, prunes them, and sets their states.
 *   These steps are described in this section.
 *
 * @param icem ICE Media object
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_checklist_form(struct icem *icem)
{
	int err;

	if (!icem)
		return EINVAL;

	if (ICE_MODE_LITE == icem->ice->lmode) {
		DEBUG_WARNING("%s: Checklist: only valid for full-mode\n",
			      icem->name);
		return EINVAL;
	}

	if (!list_isempty(&icem->checkl))
		return EALREADY;

	/* 1. form candidate pairs */
	err = candpairs_form(icem);
	if (err)
		return err;

	/* 2. compute a candidate pair priority */
	/* 3. order the pairs by priority */
	icem_candpair_prio_order(&icem->checkl);

	/* 4. prune the pairs */
	candpair_prune(icem);

	/* 5. set the pair states -- first media stream only */
	if (icem->ice->ml.head->data == icem)
		candpair_set_states(icem);

	return err;
}


/* If all of the pairs in the check list are now either in the Failed or
   Succeeded state:
 */
static bool iscompleted(const struct icem *icem)
{
	struct le *le;

	for (le = icem->checkl.head; le; le = le->next) {

		const struct ice_candpair *cp = le->data;

		if (!icem_candpair_iscompleted(cp))
			return false;
	}

	return true;
}


/* 8.  Concluding ICE Processing */
static void concluding_ice(struct icem_comp *comp)
{
	struct ice_candpair *cp;

	if (!comp || comp->concluded)
		return;

	/* pick the best candidate pair, highest priority */
	cp = icem_candpair_find_st(&comp->icem->validl, comp->id,
				   ICE_CANDPAIR_SUCCEEDED);
	if (!cp) {
		DEBUG_WARNING("{%s.%u} conclude: no valid candpair found"
			      " (validlist=%u)\n",
			      comp->icem->name, comp->id,
			      list_count(&comp->icem->validl));
		return;
	}

	icem_comp_set_selected(comp, cp);

	if (comp->icem->ice->conf.nom == ICE_NOMINATION_REGULAR) {

		/* send STUN request with USE_CAND flag via triggered qeueue */
		(void)icem_conncheck_send(cp, true, true);
		icem_conncheck_schedule_check(comp->icem);
	}

	comp->concluded = true;
}


/**
 * Check List and Timer State Updates
 */
void icem_checklist_update(struct icem *icem)
{
	struct le *le;
	bool compl;
	int err = 0;

	compl = iscompleted(icem);
	if (!compl)
		return;

	/*
	 * If there is not a pair in the valid list for each component of the
	 * media stream, the state of the check list is set to Failed.
	 */
	for (le = icem->compl.head; le; le = le->next) {

		struct icem_comp *comp = le->data;

		if (!icem_candpair_find_compid(&icem->validl, comp->id)) {
			DEBUG_WARNING("{%s.%u} no valid candidate pair"
				      " (validlist=%u)\n",
				      icem->name, comp->id,
				      list_count(&icem->validl));
			err = ENOENT;
			break;
		}

		concluding_ice(comp);

		if (!comp->cp_sel)
			continue;

		icem_comp_keepalive(comp, true);
	}

	icem->state = err ? ICE_CHECKLIST_FAILED : ICE_CHECKLIST_COMPLETED;

	if (icem->chkh) {
		icem->chkh(err, icem->ice->lrole == ROLE_CONTROLLING,
			   icem->arg);
	}
}


/**
 * Get the Local address of the Selected Candidate pair, if available
 *
 * @param icem   ICE Media object
 * @param compid Component ID
 *
 * @return Local address if available, otherwise NULL
 */
const struct sa *icem_selected_laddr(const struct icem *icem, unsigned compid)
{
	const struct icem_comp *comp = icem_comp_find(icem, compid);
	if (!comp || !comp->cp_sel)
		return NULL;

	return &comp->cp_sel->lcand->addr;
}
