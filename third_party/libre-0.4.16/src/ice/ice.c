/**
 * @file ice.c  Interactive Connectivity Establishment (ICE)
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
#include <re_sys.h>
#include <re_stun.h>
#include <re_turn.h>
#include <re_ice.h>
#include "ice.h"


/*
 * ICE Implementation as of RFC 5245
 */


static const struct ice_conf conf_default = {
	ICE_NOMINATION_REGULAR,
	ICE_DEFAULT_RTO_RTP,
	ICE_DEFAULT_RC,
	false
};


/** Determining Role */
static void ice_determine_role(struct ice *ice, bool offerer)
{
	if (!ice)
		return;

	if (ice->lmode == ice->rmode)
		ice->lrole = offerer ? ROLE_CONTROLLING : ROLE_CONTROLLED;
	else if (ice->lmode == ICE_MODE_FULL)
		ice->lrole = ROLE_CONTROLLING;
	else
		ice->lrole = ROLE_CONTROLLED;
}


static void ice_destructor(void *arg)
{
	struct ice *ice = arg;

	list_flush(&ice->ml);
	mem_deref(ice->stun);
}


/**
 * Allocate a new ICE Session
 *
 * @param icep    Pointer to allocated ICE Session object
 * @param mode    ICE Mode; Full-mode or Lite-mode
 * @param offerer True if we are SDP offerer, otherwise false
 *
 * @return 0 if success, otherwise errorcode
 */
int ice_alloc(struct ice **icep, enum ice_mode mode, bool offerer)
{
	struct ice *ice;
	int err = 0;

	if (!icep)
		return EINVAL;

	ice = mem_zalloc(sizeof(*ice), ice_destructor);
	if (!ice)
		return ENOMEM;

	list_init(&ice->ml);

	ice->conf = conf_default;
	ice->lmode = mode;
	ice->tiebrk = rand_u64();

	rand_str(ice->lufrag, sizeof(ice->lufrag));
	rand_str(ice->lpwd, sizeof(ice->lpwd));

	ice_determine_role(ice, offerer);

	if (ICE_MODE_FULL == ice->lmode) {

		err = stun_alloc(&ice->stun, NULL, NULL, NULL);
		if (err)
			goto out;

		/* Update STUN Transport */
		stun_conf(ice->stun)->rto = ice->conf.rto;
		stun_conf(ice->stun)->rc = ice->conf.rc;
	}

 out:
	if (err)
		mem_deref(ice);
	else
		*icep = ice;

	return err;
}


/**
 * Get the ICE Configuration
 *
 * @param ice ICE Session
 *
 * @return ICE Configuration
 */
struct ice_conf *ice_conf(struct ice *ice)
{
	return ice ? &ice->conf : NULL;
}


void ice_set_conf(struct ice *ice, const struct ice_conf *conf)
{
	if (!ice || !conf)
		return;

	ice->conf = *conf;

	if (ice->stun) {

		/* Update STUN Transport */
		stun_conf(ice->stun)->rto = ice->conf.rto;
		stun_conf(ice->stun)->rc = ice->conf.rc;
	}
}


/**
 * Set the offerer flag on the ICE Session
 *
 * @param ice     ICE Session
 * @param offerer True if offerer, otherwise false
 */
void ice_set_offerer(struct ice *ice, bool offerer)
{
	if (!ice)
		return;

	ice_determine_role(ice, offerer);
}


/**
 * Start the Connectivity checks on the ICE Session
 *
 * @param ice ICE Session
 *
 * @return 0 if success, otherwise errorcode
 */
int ice_conncheck_start(struct ice *ice)
{
	struct le *le;
	int err = 0;

	if (!ice)
		return EINVAL;

	for (le = ice->ml.head; le; le = le->next)
		err |= icem_conncheck_start(le->data);

	return err;
}


/**
 * Print debug information for the ICE Session
 *
 * @param pf  Print function for debug output
 * @param ice ICE Session
 *
 * @return 0 if success, otherwise errorcode
 */
int ice_debug(struct re_printf *pf, const struct ice *ice)
{
	struct le *le;
	int err = 0;

	if (!ice)
		return 0;

	err |= re_hprintf(pf, " local_mode=%s, remote_mode=%s",
			  ice_mode2name(ice->lmode),
			  ice_mode2name(ice->rmode));
	err |= re_hprintf(pf, ", local_role=%s\n", ice_role2name(ice->lrole));
	err |= re_hprintf(pf, " local_ufrag=\"%s\" local_pwd=\"%s\"\n",
			  ice->lufrag, ice->lpwd);

	for (le = ice->ml.head; le; le = le->next)
		err |= icem_debug(pf, le->data);

	err |= stun_debug(pf, ice->stun);

	return err;
}


/**
 * Get the list of ICE Media objects (struct icem) for the ICE Session
 *
 * @param ice ICE Session
 *
 * @return List of ICE Media objects
 */
struct list *ice_medialist(const struct ice *ice)
{
	return ice ? (struct list *)&ice->ml : NULL;
}


/**
 * Get the local Username fragment for the ICE Session
 *
 * @param ice ICE Session
 *
 * @return Local Username-fragment
 */
const char *ice_ufrag(const struct ice *ice)
{
	return ice ? ice->lufrag : NULL;
}


/**
 * Get the local password for the ICE Session
 *
 * @param ice ICE Session
 *
 * @return Local password
 */
const char *ice_pwd(const struct ice *ice)
{
	return ice ? ice->lpwd : NULL;
}
