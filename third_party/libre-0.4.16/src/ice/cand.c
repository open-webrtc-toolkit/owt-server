/**
 * @file cand.c  ICE Candidates
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
#include <re_sys.h>
#include <re_stun.h>
#include <re_turn.h>
#include <re_ice.h>
#include "ice.h"


#define DEBUG_MODULE "icecand"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void cand_destructor(void *arg)
{
	struct ice_cand *cand = arg;

	list_unlink(&cand->le);
	mem_deref(cand->foundation);
	mem_deref(cand->ifname);

	if (cand != cand->base)
		mem_deref(cand->base);
}


/** Foundation is a hash of IP address and candidate type */
static int compute_foundation(struct ice_cand *cand)
{
	uint32_t v;

	v  = sa_hash(&cand->addr, SA_ADDR);
	v ^= cand->type;

	return re_sdprintf(&cand->foundation, "%08x", v);
}


static int cand_alloc(struct ice_cand **candp, struct icem *icem,
		      enum ice_cand_type type, unsigned compid,
		      uint32_t prio, const char *ifname,
		      enum ice_transp transp, const struct sa *addr)
{
	struct ice_cand *cand;
	int err;

	if (!icem)
		return EINVAL;

	cand = mem_zalloc(sizeof(*cand), cand_destructor);
	if (!cand)
		return ENOMEM;

	list_append(&icem->lcandl, &cand->le, cand);

	cand->type   = type;
	cand->compid = compid;
	cand->prio   = prio;
	cand->transp = transp;

	sa_cpy(&cand->addr, addr);

	err = compute_foundation(cand);

	if (ifname)
		err |= str_dup(&cand->ifname, ifname);

	if (err)
		mem_deref(cand);
	else if (candp)
		*candp = cand;

	return err;
}


int icem_lcand_add_base(struct icem *icem, unsigned compid, uint16_t lprio,
			const char *ifname, enum ice_transp transp,
			const struct sa *addr)
{
	struct icem_comp *comp;
	struct ice_cand *cand;
	int err;

	comp = icem_comp_find(icem, compid);
	if (!comp)
		return ENOENT;

	err = cand_alloc(&cand, icem, ICE_CAND_TYPE_HOST, compid,
			 ice_cand_calc_prio(ICE_CAND_TYPE_HOST, lprio, compid),
			 ifname, transp, addr);
	if (err)
		return err;

	/* the base is itself */
	cand->base = cand;

	sa_set_port(&cand->addr, comp->lport);

	return 0;
}


int icem_lcand_add(struct icem *icem, struct ice_cand *base,
		   enum ice_cand_type type,
		   const struct sa *addr)
{
	struct ice_cand *cand;
	int err;

	if (!base)
		return EINVAL;

	err = cand_alloc(&cand, icem, type, base->compid,
			 ice_cand_calc_prio(type, 0, base->compid),
			 base->ifname, base->transp, addr);
	if (err)
		return err;

	cand->base = mem_ref(base);
	sa_cpy(&cand->rel, &base->addr);

	return 0;
}


int icem_rcand_add(struct icem *icem, enum ice_cand_type type, unsigned compid,
		   uint32_t prio, const struct sa *addr,
		   const struct sa *rel_addr, const struct pl *foundation)
{
	struct ice_cand *rcand;
	int err;

	if (!icem || !foundation)
		return EINVAL;

	rcand = mem_zalloc(sizeof(*rcand), cand_destructor);
	if (!rcand)
		return ENOMEM;

	list_append(&icem->rcandl, &rcand->le, rcand);

	rcand->type   = type;
	rcand->compid = compid;
	rcand->prio   = prio;

	sa_cpy(&rcand->addr, addr);
	sa_cpy(&rcand->rel, rel_addr);

	err = pl_strdup(&rcand->foundation, foundation);

	if (err)
		mem_deref(rcand);

	return err;
}


int icem_rcand_add_prflx(struct ice_cand **rcp, struct icem *icem,
			 unsigned compid, uint32_t prio,
			 const struct sa *addr)
{
	struct ice_cand *rcand;
	int err;

	if (!icem || !addr)
		return EINVAL;

	rcand = mem_zalloc(sizeof(*rcand), cand_destructor);
	if (!rcand)
		return ENOMEM;

	list_append(&icem->rcandl, &rcand->le, rcand);

	rcand->type   = ICE_CAND_TYPE_PRFLX;
	rcand->compid = compid;
	rcand->prio   = prio;
	rcand->addr   = *addr;

	err = re_sdprintf(&rcand->foundation, "%08x", rand_u32());
	if (err)
		goto out;

	icecomp_printf(icem_comp_find(icem, compid),
		       "added PeerReflexive remote candidate"
		       " with priority %u (%J)\n", prio, addr);

 out:
	if (err)
		mem_deref(rcand);
	else if (rcp)
		*rcp = rcand;

	return err;
}


struct ice_cand *icem_cand_find(const struct list *lst, unsigned compid,
				const struct sa *addr)
{
	struct le *le;

	for (le = list_head(lst); le; le = le->next) {

		struct ice_cand *cand = le->data;

		if (compid && cand->compid != compid)
			continue;

		if (addr && !sa_cmp(&cand->addr, addr, SA_ALL))
			continue;

		return cand;
	}

	return NULL;
}


/**
 * Find the highest priority LCAND on the check-list of type HOST/RELAY
 */
struct ice_cand *icem_lcand_find_checklist(const struct icem *icem,
					   unsigned compid)
{
	struct le *le;

	for (le = icem->checkl.head; le; le = le->next) {
		struct ice_candpair *cp = le->data;

		if (cp->lcand->compid != compid)
			continue;

		switch (cp->lcand->type) {

		case ICE_CAND_TYPE_HOST:
		case ICE_CAND_TYPE_RELAY:
			return cp->lcand;

		default:
			break;
		}
	}

	return NULL;
}


int icem_cands_debug(struct re_printf *pf, const struct list *lst)
{
	struct le *le;
	int err;

	err = re_hprintf(pf, " (%u)\n", list_count(lst));

	for (le = list_head(lst); le && !err; le = le->next) {

		const struct ice_cand *cand = le->data;

		err |= re_hprintf(pf, "  {%u} fnd=%-2s prio=%08x %24H",
				  cand->compid, cand->foundation, cand->prio,
				  icem_cand_print, cand);

		if (sa_isset(&cand->rel, SA_ADDR))
			err |= re_hprintf(pf, " (rel-addr=%J)", &cand->rel);

		err |= re_hprintf(pf, "\n");
	}

	return err;
}


int icem_cand_print(struct re_printf *pf, const struct ice_cand *cand)
{
	int err = 0;

	if (!cand)
		return 0;

	if (cand->ifname)
		err |= re_hprintf(pf, "%s:", cand->ifname);

	err |= re_hprintf(pf, "%s:%J",
			  ice_cand_type2name(cand->type), &cand->addr);

	return err;
}
