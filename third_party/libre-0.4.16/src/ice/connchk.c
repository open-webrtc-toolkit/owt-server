/**
 * @file connchk.c  ICE Connectivity Checks
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


#define DEBUG_MODULE "connchk"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void pace_next(struct icem *icem)
{
	if (icem->state != ICE_CHECKLIST_RUNNING)
		return;

	icem_conncheck_schedule_check(icem);

	if (icem->state == ICE_CHECKLIST_FAILED)
		return;

	icem_checklist_update(icem);
}


/**
 * Constructing a Valid Pair
 *
 * @return The valid pair
 */
static struct ice_candpair *construct_valid_pair(struct icem *icem,
					     struct ice_candpair *cp,
					     const struct sa *mapped,
					     const struct sa *dest)
{
	struct ice_cand *lcand, *rcand;
	struct ice_candpair *cp2;
	int err;

	lcand = icem_cand_find(&icem->lcandl, cp->lcand->compid, mapped);
	rcand = icem_cand_find(&icem->rcandl, cp->rcand->compid, dest);

	if (!lcand) {
		DEBUG_WARNING("no such local candidate: %J\n", mapped);
		return NULL;
	}
	if (!rcand) {
		DEBUG_WARNING("no such remote candidate: %J\n", dest);
		return NULL;
	}

	/* New candidate? -- implicit success */
	if (lcand != cp->lcand || rcand != cp->rcand) {

		if (lcand != cp->lcand) {
			icecomp_printf(cp->comp,
				       "New local candidate for mapped %J\n",
				       mapped);
		}
		if (rcand != cp->rcand) {
			icecomp_printf(cp->comp,
				       "New remote candidate for dest %J\n",
				       dest);
		}

		/* The original candidate pair is set to 'Failed' because
		 * the implicitly discovered pair is 'better'.
		 * This happens for UAs behind NAT where the original
		 * pair is of type 'host' and the implicit pair is 'srflx'
		 */

		icem_candpair_make_valid(cp);

		cp2 = icem_candpair_find(&icem->validl, lcand, rcand);
		if (cp2)
			return cp2;

		err = icem_candpair_clone(&cp2, cp, lcand, rcand);
		if (err)
			return NULL;

		icem_candpair_make_valid(cp2);
		/*icem_candpair_failed(cp, EINTR, 0);*/

		return cp2;
	}
	else {
		/* Add to VALID LIST, the pair that generated the check */
		icem_candpair_make_valid(cp);

		return cp;
	}
}


static void handle_success(struct icem *icem, struct ice_candpair *cp,
			   const struct sa *laddr)
{
	if (!icem_cand_find(&icem->lcandl, cp->lcand->compid, laddr)) {

		int err;

		icecomp_printf(cp->comp, "adding local PRFLX Candidate: %J\n",
			       laddr);

		err = icem_lcand_add(icem, cp->lcand,
				     ICE_CAND_TYPE_PRFLX, laddr);
		if (err) {
			DEBUG_WARNING("failed to add PRFLX: %m\n", err);
		}
	}

	cp = construct_valid_pair(icem, cp, laddr, &cp->rcand->addr);
	if (!cp) {
		DEBUG_WARNING("{%s} no valid candidate pair for %J\n",
			      icem->name, laddr);
		return;
	}

	icem_candpair_make_valid(cp);
	icem_comp_set_selected(cp->comp, cp);

	cp->nominated = true;

#if 0
	/* stop conncheck now -- conclude */
	icem_conncheck_stop(icem, 0);
#endif
}


#if ICE_TRACE
static int print_err(struct re_printf *pf, const int *err)
{
	if (err && *err)
		return re_hprintf(pf, " (%m)", *err);

	return 0;
}
#endif


static void stunc_resp_handler(int err, uint16_t scode, const char *reason,
			       const struct stun_msg *msg, void *arg)
{
	struct ice_candpair *cp = arg;
	struct icem *icem = cp->icem;
	struct stun_attr *attr;

	(void)reason;

#if ICE_TRACE
	icecomp_printf(cp->comp, "Rx %H <--- %H '%u %s'%H\n",
		       icem_cand_print, cp->lcand,
		       icem_cand_print, cp->rcand,
		       scode, reason, print_err, &err);
#endif

	if (err) {
		icem_candpair_failed(cp, err, scode);
		goto out;
	}

	switch (scode) {

	case 0: /* Success case */
		attr = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
		if (!attr) {
			DEBUG_WARNING("no XOR-MAPPED-ADDR in response\n");
			icem_candpair_failed(cp, EBADMSG, 0);
			break;
		}

		handle_success(icem, cp, &attr->v.sa);
		break;

	case 487: /* Role Conflict */
		ice_switch_local_role(icem->ice);
		(void)icem_conncheck_send(cp, false, true);
		break;

	default:
		DEBUG_WARNING("{%s.%u} STUN Response: %u %s\n",
			      icem->name, cp->comp->id, scode, reason);
		icem_candpair_failed(cp, err, scode);
		break;
	}

 out:
	pace_next(icem);
}


int icem_conncheck_send(struct ice_candpair *cp, bool use_cand, bool trigged)
{
	struct ice_cand *lcand = cp->lcand;
	struct icem *icem = cp->icem;
	struct ice *ice = icem->ice;
	char username_buf[64];
	size_t presz = 0;
	uint32_t prio_prflx;
	uint16_t ctrl_attr;
	int err = 0;

	icem_candpair_set_state(cp, ICE_CANDPAIR_INPROGRESS);

	(void)re_snprintf(username_buf, sizeof(username_buf),
			  "%s:%s", icem->rufrag, ice->lufrag);

	/* PRIORITY and USE-CANDIDATE */
	prio_prflx = ice_cand_calc_prio(ICE_CAND_TYPE_PRFLX, 0, lcand->compid);

	switch (ice->lrole) {

	case ROLE_CONTROLLING:
		ctrl_attr = STUN_ATTR_CONTROLLING;

		if (ice->conf.nom == ICE_NOMINATION_AGGRESSIVE)
			use_cand = true;
		break;

	case ROLE_CONTROLLED:
		ctrl_attr = STUN_ATTR_CONTROLLED;
		break;

	default:
		return EINVAL;
	}

#if ICE_TRACE
	icecomp_printf(cp->comp, "Tx %H ---> %H (%s) %s %s\n",
		       icem_cand_print, cp->lcand, icem_cand_print, cp->rcand,
		       ice_candpair_state2name(cp->state),
		       use_cand ? "[USE]" : "",
		       trigged ? "[Trigged]" : "");
#else
	(void)trigged;
#endif

	/* A connectivity check MUST utilize the STUN short term credential
	   mechanism. */

	/* The password is equal to the password provided by the peer */
	if (!icem->rpwd) {
		DEBUG_WARNING("no remote password!\n");
	}

	if (cp->ct_conn) {
		DEBUG_WARNING("send_req: CONNCHECK already Pending!\n");
		return EBUSY;
	}

	switch (lcand->type) {

	case ICE_CAND_TYPE_RELAY:
		/* Creating Permissions for Relayed Candidates */
		err = turnc_add_chan(cp->comp->turnc, &cp->rcand->addr,
				     NULL, NULL);
		if (err) {
			DEBUG_WARNING("add channel: %m\n", err);
			break;
		}
		presz = 4;
		/*@fallthrough@*/

	case ICE_CAND_TYPE_HOST:
	case ICE_CAND_TYPE_SRFLX:
	case ICE_CAND_TYPE_PRFLX:
		cp->ct_conn = mem_deref(cp->ct_conn);
		err = stun_request(&cp->ct_conn, ice->stun, icem->proto,
				   cp->comp->sock, &cp->rcand->addr, presz,
				   STUN_METHOD_BINDING,
				   (uint8_t *)icem->rpwd, str_len(icem->rpwd),
				   true, stunc_resp_handler, cp,
				   4,
				   STUN_ATTR_USERNAME, username_buf,
				   STUN_ATTR_PRIORITY, &prio_prflx,
				   ctrl_attr, &ice->tiebrk,
				   STUN_ATTR_USE_CAND,
				   use_cand ? &use_cand : 0);
		break;

	default:
		DEBUG_WARNING("unknown candidate type %d\n", lcand->type);
		err = EINVAL;
		break;
	}

	return err;
}


static void abort_ice(struct icem *icem, int err)
{
	icem->state = ICE_CHECKLIST_FAILED;
	tmr_cancel(&icem->tmr_pace);

	if (icem->chkh) {
		icem->chkh(err, icem->ice->lrole == ROLE_CONTROLLING,
			   icem->arg);
	}

	icem->chkh = NULL;
}


static void do_check(struct ice_candpair *cp)
{
	int err;

	err = icem_conncheck_send(cp, false, false);
	if (err) {
		icem_candpair_failed(cp, err, 0);

		if (err == ENOMEM) {
			abort_ice(cp->icem, err);
		}
		else {
			pace_next(cp->icem);
		}
	}
}


/**
 * Scheduling Checks
 */
void icem_conncheck_schedule_check(struct icem *icem)
{
	struct ice_candpair *cp;

	/* Find the highest priority pair in that check list that is in the
	   Waiting state. */
	cp = icem_candpair_find_st(&icem->checkl, 0, ICE_CANDPAIR_WAITING);
	if (cp) {
		do_check(cp);
		return;
	}

	/* If there is no such pair: */

	/* Find the highest priority pair in that check list that is in
	   the Frozen state. */
	cp = icem_candpair_find_st(&icem->checkl, 0, ICE_CANDPAIR_FROZEN);
	if (cp) { /* If there is such a pair: */

		/* Unfreeze the pair.
		   Perform a check for that pair, causing its state to
		   transition to In-Progress. */
		do_check(cp);
		return;
	}

	/* If there is no such pair: */

	/* Terminate the timer for that check list. */

#if 0
	icem->state = ICE_CHECKLIST_COMPLETED;
#endif
}


static void pace_timeout(void *arg)
{
	struct icem *icem = arg;

	pace_next(icem);
}


/**
 * Scheduling Checks
 */
int icem_conncheck_start(struct icem *icem)
{
	int err;

	if (!icem)
		return EINVAL;

	if (ICE_MODE_FULL != icem->ice->lmode)
		return EINVAL;

	err = icem_checklist_form(icem);
	if (err)
		return err;

	icem->state = ICE_CHECKLIST_RUNNING;

	icem_printf(icem, "starting connectivity checks"
		    " with %u candidate pairs\n",
		    list_count(&icem->checkl));

	/* add some delay, to wait for call to be 'established' */
	tmr_start(&icem->tmr_pace, 10, pace_timeout, icem);

	return 0;
}


void icem_conncheck_continue(struct icem *icem)
{
	if (!tmr_isrunning(&icem->tmr_pace))
		tmr_start(&icem->tmr_pace, 1, pace_timeout, icem);
}


/**
 * Stop checklist, cancel all connectivity checks
 */
void icem_conncheck_stop(struct icem *icem, int err)
{
	struct le *le;

	icem->state = err ? ICE_CHECKLIST_FAILED : ICE_CHECKLIST_COMPLETED;

	tmr_cancel(&icem->tmr_pace);

	for (le = icem->checkl.head; le; le = le->next) {
		struct ice_candpair *cp = le->data;

		if (!icem_candpair_iscompleted(cp)) {
			icem_candpair_cancel(cp);
			icem_candpair_failed(cp, EINTR, 0);
		}
	}

	icem_checklist_update(icem);
}
