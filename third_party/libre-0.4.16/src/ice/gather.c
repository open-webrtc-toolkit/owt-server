/**
 * @file gather.c  ICE Candidate Gathering
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


#define DEBUG_MODULE "icegath"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void call_gather_handler(int err, struct icem *icem, uint16_t scode,
				const char *reason)
{
	struct le *le;

	/* No more pending requests? */
	if (icem->nstun != 0)
		return;

	if (!icem->gh)
		return;

	if (err)
		goto out;

	icem_cand_redund_elim(icem);

	for (le = icem->compl.head; le; le = le->next) {

		struct icem_comp *comp = le->data;

		err |= icem_comp_set_default_cand(comp);
	}

 out:
	icem->gh(err, scode, reason, icem->arg);

	icem->gh = NULL;
}


static void stun_resp_handler(int err, uint16_t scode, const char *reason,
			      const struct stun_msg *msg, void *arg)
{
	struct icem_comp *comp = arg;
	struct icem *icem = comp->icem;
	struct stun_attr *attr;
	struct ice_cand *lcand;

	--icem->nstun;

	if (err || scode > 0) {
		DEBUG_WARNING("{%s.%u} STUN Request failed: %m\n",
			      icem->name, comp->id, err);
		goto out;
	}

	lcand = icem_cand_find(&icem->lcandl, comp->id, NULL);
	if (!lcand)
		goto out;

	attr = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
	if (!attr)
		attr = stun_msg_attr(msg, STUN_ATTR_MAPPED_ADDR);
	if (!attr) {
		DEBUG_WARNING("no Mapped Address in Response\n");
		err = EPROTO;
		goto out;
	}

	err = icem_lcand_add(icem, lcand->base, ICE_CAND_TYPE_SRFLX,
			     &attr->v.sa);

 out:
	call_gather_handler(err, icem, scode, reason);
}


/** Gather Server Reflexive address */
static int send_binding_request(struct icem *icem, struct icem_comp *comp)
{
	int err;

	if (comp->ct_gath)
		return EALREADY;

	err = stun_request(&comp->ct_gath, icem->ice->stun, icem->proto,
			   comp->sock, &icem->stun_srv, 0,
			   STUN_METHOD_BINDING,
			   NULL, false, 0,
			   stun_resp_handler, comp, 1,
			   STUN_ATTR_SOFTWARE, stun_software);
	if (err)
		return err;

	++icem->nstun;

	return 0;
}


static void turnc_handler(int err, uint16_t scode, const char *reason,
			  const struct sa *relay, const struct sa *mapped,
			  const struct stun_msg *msg, void *arg)
{
	struct icem_comp *comp = arg;
	struct icem *icem = comp->icem;
	struct ice_cand *lcand;
	(void)msg;

	--icem->nstun;

	/* TURN failed, so we destroy the client */
	if (err || scode) {
		comp->turnc = mem_deref(comp->turnc);
	}

	if (err) {
		DEBUG_WARNING("{%s.%u} TURN Client error: %m\n",
			      icem->name, comp->id, err);
		goto out;
	}

	if (scode) {
		DEBUG_WARNING("{%s.%u} TURN Client error: %u %s\n",
			      icem->name, comp->id, scode, reason);
		err = send_binding_request(icem, comp);
		if (err)
			goto out;
		return;
	}

	lcand = icem_cand_find(&icem->lcandl, comp->id, NULL);
	if (!lcand)
		goto out;

	if (!sa_cmp(relay, &lcand->base->addr, SA_ALL)) {
		err = icem_lcand_add(icem, lcand->base,
				     ICE_CAND_TYPE_RELAY, relay);
	}

	if (mapped) {
		err |= icem_lcand_add(icem, lcand->base,
				      ICE_CAND_TYPE_SRFLX, mapped);
	}
	else {
		err |= send_binding_request(icem, comp);
	}

 out:
	call_gather_handler(err, icem, scode, reason);
}


static int cand_gather_relayed(struct icem *icem, struct icem_comp *comp,
			       const char *username, const char *password)
{
	const int layer = icem->layer - 10; /* below ICE stack */
	int err;

	if (comp->turnc)
		return EALREADY;

	err = turnc_alloc(&comp->turnc, stun_conf(icem->ice->stun),
			  icem->proto, comp->sock, layer, &icem->stun_srv,
			  username, password,
			  60, turnc_handler, comp);
	if (err)
		return err;

	++icem->nstun;

	return 0;
}


static int start_gathering(struct icem *icem, const struct sa *stun_srv,
			   const char *username, const char *password)
{
	struct le *le;
	int err = 0;

	if (icem->ice->lmode != ICE_MODE_FULL)
		return EINVAL;

	if (list_isempty(&icem->compl)) {
		DEBUG_WARNING("gathering: no components for"
			      " mediastream '%s'\n", icem->name);
		return ENOENT;
	}

	sa_cpy(&icem->stun_srv, stun_srv);

	/* for each component */
	for (le = icem->compl.head; le; le = le->next) {

		struct icem_comp *comp = le->data;

		if (username && password) {
			err |= cand_gather_relayed(icem, comp,
						   username, password);
		}
		else
			err |= send_binding_request(icem, comp);
	}

	return err;
}


/**
 * Gather Server Reflexive candidates using STUN Server
 *
 * @param icem      ICE Media object
 * @param stun_srv  STUN Server network address
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_gather_srflx(struct icem *icem, const struct sa *stun_srv)
{
	if (!icem || !stun_srv)
		return EINVAL;

	return start_gathering(icem, stun_srv, NULL, NULL);
}


/**
 * Gather Relayed and Server Reflexive candidates using TURN Server
 *
 * @param icem      ICE Media object
 * @param stun_srv  TURN Server network address
 * @param username  TURN Server username
 * @param password  TURN Server password
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_gather_relay(struct icem *icem, const struct sa *stun_srv,
		      const char *username, const char *password)
{
	if (!icem || !stun_srv || !username || !password)
		return EINVAL;

	return start_gathering(icem, stun_srv, username, password);
}
