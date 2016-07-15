/**
 * @file candpair.c  ICE Candidate Pairs
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


#define DEBUG_MODULE "cndpair"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void candpair_destructor(void *arg)
{
	struct ice_candpair *cp = arg;

	list_unlink(&cp->le);
	mem_deref(cp->ct_conn);
	mem_deref(cp->lcand);
	mem_deref(cp->rcand);
}


static bool sort_handler(struct le *le1, struct le *le2, void *arg)
{
	const struct ice_candpair *cp1 = le1->data, *cp2 = le2->data;
	(void)arg;

	return cp1->pprio >= cp2->pprio;
}


static void candpair_set_pprio(struct ice_candpair *cp)
{
	uint32_t g, d;

	if (ROLE_CONTROLLING == cp->icem->ice->lrole) {
		g = cp->lcand->prio;
		d = cp->rcand->prio;
	}
	else {
		g = cp->rcand->prio;
		d = cp->lcand->prio;
	}

	cp->pprio = ice_calc_pair_prio(g, d);
}


/**
 * Add candidate pair to list, sorted by pair priority (highest is first)
 */
static void list_add_sorted(struct list *list, struct ice_candpair *cp)
{
	struct le *le;

	/* find our slot */
	for (le = list_tail(list); le; le = le->prev) {
		struct ice_candpair *cp0 = le->data;

		if (cp->pprio < cp0->pprio) {
			list_insert_after(list, le, &cp->le, cp);
			return;
		}
	}

	list_prepend(list, &cp->le, cp);
}


int icem_candpair_alloc(struct ice_candpair **cpp, struct icem *icem,
			struct ice_cand *lcand, struct ice_cand *rcand)
{
	struct ice_candpair *cp;
	struct icem_comp *comp;

	if (!icem || !lcand || !rcand)
		return EINVAL;

	comp = icem_comp_find(icem, lcand->compid);
	if (!comp)
		return ENOENT;

	cp = mem_zalloc(sizeof(*cp), candpair_destructor);
	if (!cp)
		return ENOMEM;

	cp->icem  = icem;
	cp->comp  = comp;
	cp->lcand = mem_ref(lcand);
	cp->rcand = mem_ref(rcand);
	cp->state = ICE_CANDPAIR_FROZEN;
	cp->def   = comp->def_lcand == lcand && comp->def_rcand == rcand;

	candpair_set_pprio(cp);

	list_add_sorted(&icem->checkl, cp);

	if (cpp)
		*cpp = cp;

	return 0;
}


int icem_candpair_clone(struct ice_candpair **cpp, struct ice_candpair *cp0,
			struct ice_cand *lcand, struct ice_cand *rcand)
{
	struct ice_candpair *cp;

	if (!cp0)
		return EINVAL;

	cp = mem_zalloc(sizeof(*cp), candpair_destructor);
	if (!cp)
		return ENOMEM;

	cp->icem      = cp0->icem;
	cp->comp      = cp0->comp;
	cp->lcand     = mem_ref(lcand ? lcand : cp0->lcand);
	cp->rcand     = mem_ref(rcand ? rcand : cp0->rcand);
	cp->def       = cp0->def;
	cp->valid     = cp0->valid;
	cp->nominated = cp0->nominated;
	cp->state     = cp0->state;
	cp->pprio     = cp0->pprio;
	cp->err       = cp0->err;
	cp->scode     = cp0->scode;

	list_add_sorted(&cp0->icem->checkl, cp);

	if (cpp)
		*cpp = cp;

	return 0;
}


/** Computing Pair Priority and Ordering Pairs */
void icem_candpair_prio_order(struct list *lst)
{
	struct le *le;

	for (le = list_head(lst); le; le = le->next) {
		struct ice_candpair *cp = le->data;

		candpair_set_pprio(cp);
	}

	list_sort(lst, sort_handler, NULL);
}


/* cancel transaction */
void icem_candpair_cancel(struct ice_candpair *cp)
{
	if (!cp)
		return;

	cp->ct_conn = mem_deref(cp->ct_conn);
}


void icem_candpair_make_valid(struct ice_candpair *cp)
{
	if (!cp)
		return;

	cp->err = 0;
	cp->scode = 0;
	cp->valid = true;

	icem_candpair_set_state(cp, ICE_CANDPAIR_SUCCEEDED);

	list_unlink(&cp->le);
	list_add_sorted(&cp->icem->validl, cp);
}


void icem_candpair_failed(struct ice_candpair *cp, int err, uint16_t scode)
{
	if (!cp)
		return;

	cp->err = err;
	cp->scode = scode;
	cp->valid = false;

	icem_candpair_set_state(cp, ICE_CANDPAIR_FAILED);
}


void icem_candpair_set_state(struct ice_candpair *cp,
			     enum ice_candpair_state state)
{
	if (!cp)
		return;
	if (cp->state == state || icem_candpair_iscompleted(cp))
		return;

	icecomp_printf(cp->comp,
		       "%5s <---> %5s  FSM:  %10s ===> %-10s\n",
		       ice_cand_type2name(cp->lcand->type),
		       ice_cand_type2name(cp->rcand->type),
		       ice_candpair_state2name(cp->state),
		       ice_candpair_state2name(state));

	cp->state = state;
}


/**
 * Delete all Candidate-Pairs where the Local candidate is of a given type
 */
void icem_candpairs_flush(struct list *lst, enum ice_cand_type type,
			  unsigned compid)
{
	struct le *le = list_head(lst);

	while (le) {

		struct ice_candpair *cp = le->data;

		le = le->next;

		if (cp->lcand->compid != compid)
			continue;

		if (cp->lcand->type != type)
			continue;

		mem_deref(cp);
	}
}


bool icem_candpair_iscompleted(const struct ice_candpair *cp)
{
	if (!cp)
		return false;

	return cp->state == ICE_CANDPAIR_FAILED ||
		cp->state == ICE_CANDPAIR_SUCCEEDED;
}


/**
 * Compare local and remote candidates of two candidate pairs
 *
 * @param cp1  First Candidate pair
 * @param cp2  Second Candidate pair
 *
 * @return true if match
 */
bool icem_candpair_cmp(const struct ice_candpair *cp1,
		       const struct ice_candpair *cp2)
{
	if (!sa_cmp(&cp1->lcand->addr, &cp2->lcand->addr, SA_ALL))
		return false;

	return sa_cmp(&cp1->rcand->addr, &cp2->rcand->addr, SA_ALL);
}


/**
 * Find the highest-priority candidate-pair in a given list, with
 * optional match parameters
 *
 * @param lst    List of candidate pairs
 * @param lcand  Local candidate (optional)
 * @param rcand  Remote candidate (optional)
 *
 * @return Matching candidate pair if found, otherwise NULL
 *
 * note: assume list is sorted by priority
 */
struct ice_candpair *icem_candpair_find(const struct list *lst,
				    const struct ice_cand *lcand,
				    const struct ice_cand *rcand)
{
	struct le *le;

	for (le = list_head(lst); le; le = le->next) {

		struct ice_candpair *cp = le->data;

		if (!cp->lcand || !cp->rcand) {
			DEBUG_WARNING("corrupt candpair %p\n", cp);
			continue;
		}

		if (lcand && cp->lcand != lcand)
			continue;

		if (rcand && cp->rcand != rcand)
			continue;

		return cp;
	}

	return NULL;
}


struct ice_candpair *icem_candpair_find_st(const struct list *lst,
					   unsigned compid,
					   enum ice_candpair_state state)
{
	struct le *le;

	for (le = list_head(lst); le; le = le->next) {

		struct ice_candpair *cp = le->data;

		if (compid && cp->lcand->compid != compid)
			continue;

		if (cp->state != state)
			continue;

		return cp;
	}

	return NULL;
}


struct ice_candpair *icem_candpair_find_compid(const struct list *lst,
					   unsigned compid)
{
	struct le *le;

	for (le = list_head(lst); le; le = le->next) {

		struct ice_candpair *cp = le->data;

		if (cp->lcand->compid != compid)
			continue;

		return cp;
	}

	return NULL;
}


/**
 * Find a remote candidate in the checklist or validlist
 */
struct ice_candpair *icem_candpair_find_rcand(struct icem *icem,
					  const struct ice_cand *rcand)
{
	struct ice_candpair *cp;

	cp = icem_candpair_find(&icem->checkl, NULL, rcand);
	if (cp)
		return cp;

	cp = icem_candpair_find(&icem->validl, NULL, rcand);
	if (cp)
		return cp;

	return NULL;
}


bool icem_candpair_cmp_fnd(const struct ice_candpair *cp1,
			   const struct ice_candpair *cp2)
{
	if (!cp1 || !cp2)
		return false;

	return 0 == strcmp(cp1->lcand->foundation, cp2->lcand->foundation) &&
		0 == strcmp(cp1->rcand->foundation, cp2->rcand->foundation);
}


int icem_candpair_debug(struct re_printf *pf, const struct ice_candpair *cp)
{
	int err;

	if (!cp)
		return 0;

	err = re_hprintf(pf, "{comp=%u} %10s {%c%c%c} %28H <---> %28H",
			 cp->lcand->compid,
			 ice_candpair_state2name(cp->state),
			 cp->def ? 'D' : ' ',
			 cp->valid ? 'V' : ' ',
			 cp->nominated ? 'N' : ' ',
			 icem_cand_print, cp->lcand,
			 icem_cand_print, cp->rcand);

	if (cp->err)
		err |= re_hprintf(pf, " (%m)", cp->err);

	if (cp->scode)
		err |= re_hprintf(pf, " [%u]", cp->scode);

	return err;
}


int icem_candpairs_debug(struct re_printf *pf, const struct list *list)
{
	struct le *le;
	int err;

	if (!list)
		return 0;

	err = re_hprintf(pf, " (%u)\n", list_count(list));

	for (le = list->head; le && !err; le = le->next) {

		const struct ice_candpair *cp = le->data;
		bool is_selected = (cp == cp->comp->cp_sel);

		err = re_hprintf(pf, "  %c  %H\n",
				 is_selected ? '*' : ' ',
				 icem_candpair_debug, cp);
	}

	return err;
}
