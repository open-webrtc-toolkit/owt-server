/**
 * @file icesdp.c  SDP Attributes for ICE
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
#include <re_net.h>
#include <re_stun.h>
#include <re_ice.h>
#include "ice.h"


#define DEBUG_MODULE "icesdp"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


const char ice_attr_cand[]        = "candidate";
const char ice_attr_remote_cand[] = "remote-candidates";
const char ice_attr_lite[]        = "ice-lite";
const char ice_attr_ufrag[]       = "ice-ufrag";
const char ice_attr_pwd[]         = "ice-pwd";
const char ice_attr_mismatch[]    = "ice-mismatch";


static const char rel_addr_str[] = "raddr";
static const char rel_port_str[] = "rport";


/* Encode SDP Attributes */


static const char *transp_name(enum ice_transp transp)
{
	switch (transp) {

	case ICE_TRANSP_UDP: return "UDP";
	default:             return "???";
	}
}


static enum ice_transp transp_resolve(const struct pl *transp)
{
	if (!pl_strcasecmp(transp, "UDP"))
		return ICE_TRANSP_UDP;

	return ICE_TRANSP_NONE;
}


/**
 * Encode SDP candidate attribute
 *
 * @param pf    Print function
 * @param cand  Candidate to encode
 *
 * @return 0 if success, otherwise errorcode
 */
int ice_cand_encode(struct re_printf *pf, const struct ice_cand *cand)
{
	int err;

	err = re_hprintf(pf, "%s %u %s %u %j %u typ %s",
			 cand->foundation, cand->compid,
			 transp_name(cand->transp), cand->prio,
			 &cand->addr, sa_port(&cand->addr),
			 ice_cand_type2name(cand->type));

	if (sa_isset(&cand->rel, SA_ADDR))
		err |= re_hprintf(pf, " raddr %j", &cand->rel);

	if (sa_isset(&cand->rel, SA_PORT))
		err |= re_hprintf(pf, " rport %u", sa_port(&cand->rel));

	return err;
}


/**
 * Check if remote candidates are available
 *
 * @param icem ICE Media object
 *
 * @return True if available, otherwise false
 */
bool ice_remotecands_avail(const struct icem *icem)
{
	if (!icem)
		return false;

	return icem->ice->lrole == ROLE_CONTROLLING &&
		icem->state == ICE_CHECKLIST_COMPLETED;
}


/**
 * Encode the SDP "remote-candidates" Attribute
 *
 * @param pf   Print function
 * @param icem ICE Media object
 *
 * @return 0 if success, otherwise errorcode
 */
int ice_remotecands_encode(struct re_printf *pf, const struct icem *icem)
{
	struct le *le;
	int err = 0;

	if (!icem)
		return EINVAL;

	for (le = icem->rcandl.head; le && !err; le = le->next) {

		const struct ice_cand *rcand = le->data;

		err = re_hprintf(pf, "%s%d %j %u",
				 icem->rcandl.head==le ? "" : " ",
				 rcand->compid,
				 &rcand->addr, sa_port(&rcand->addr));
	}

	return err;
}


/* Decode SDP Attributes */


static int ufrag_decode(struct ice *ice, const char *value)
{
	char *ufrag = NULL;
	struct le *le;
	int err;

	err = str_dup(&ufrag, value);
	if (err)
		return err;

	for (le = ice->ml.head; le; le = le->next) {

		struct icem *icem = le->data;

		mem_deref(icem->rufrag);
		icem->rufrag = mem_ref(ufrag);
	}

	mem_deref(ufrag);

	return 0;
}


static int pwd_decode(struct ice *ice, const char *value)
{
	char *pwd = NULL;
	struct le *le;
	int err;

	err = str_dup(&pwd, value);
	if (err)
		return err;

	for (le = ice->ml.head; le; le = le->next) {

		struct icem *icem = le->data;

		mem_deref(icem->rpwd);
		icem->rpwd = mem_ref(pwd);
	}

	mem_deref(pwd);

	return 0;
}


static int media_ufrag_decode(struct icem *icem, const char *value)
{
	icem->rufrag = mem_deref(icem->rufrag);

	return str_dup(&icem->rufrag, value);
}


static int media_pwd_decode(struct icem *icem, const char *value)
{
	icem->rpwd = mem_deref(icem->rpwd);

	return str_dup(&icem->rpwd, value);
}


static int cand_decode(struct icem *icem, const char *val)
{
	struct pl foundation, compid, transp, prio, addr, port, cand_type;
	struct pl extra = pl_null;
	struct sa caddr, rel_addr;
	char type[8];
	uint8_t cid;
	int err;

	sa_init(&rel_addr, AF_INET);

	err = re_regex(val, strlen(val),
		       "[^ ]+ [0-9]+ [^ ]+ [0-9]+ [^ ]+ [0-9]+ typ [a-z]+[^]*",
		       &foundation, &compid, &transp, &prio,
		       &addr, &port, &cand_type, &extra);
	if (err)
		return err;

	if (ICE_TRANSP_NONE == transp_resolve(&transp)) {
		DEBUG_NOTICE("<%s> ignoring candidate with"
			     " unknown transport=%r (%r:%r)\n",
			     icem->name, &transp, &cand_type, &addr);
		return 0;
	}

	if (pl_isset(&extra)) {

		struct pl name, value;

		/* Loop through " SP attr SP value" pairs */
		while (!re_regex(extra.p, extra.l, " [^ ]+ [^ ]+",
				 &name, &value)) {

			pl_advance(&extra, value.p + value.l - extra.p);

			if (0 == pl_strcasecmp(&name, rel_addr_str)) {
				err = sa_set(&rel_addr, &value,
					     sa_port(&rel_addr));
				if (err)
					break;
			}
			else if (0 == pl_strcasecmp(&name, rel_port_str)) {
				sa_set_port(&rel_addr, pl_u32(&value));
			}
		}
	}

	err = sa_set(&caddr, &addr, pl_u32(&port));
	if (err)
		return err;

	cid = pl_u32(&compid);

	/* add only if not exist */
	if (icem_cand_find(&icem->rcandl, cid, &caddr))
		return 0;

	(void)pl_strcpy(&cand_type, type, sizeof(type));

	return icem_rcand_add(icem, ice_cand_name2type(type), cid,
			      pl_u32(&prio), &caddr, &rel_addr, &foundation);
}


/**
 * Decode SDP session attributes
 *
 * @param ice   ICE Session
 * @param name  Name of the SDP attribute
 * @param value Value of the SDP attribute (optional)
 *
 * @return 0 if success, otherwise errorcode
 */
int ice_sdp_decode(struct ice *ice, const char *name, const char *value)
{
	if (!ice)
		return EINVAL;

	if (0 == str_casecmp(name, ice_attr_lite)) {
		if (ICE_MODE_LITE == ice->lmode) {
			DEBUG_WARNING("we are lite, peer is also lite!\n");
			return EPROTO;
		}
		ice->rmode = ICE_MODE_LITE;
		ice->lrole = ROLE_CONTROLLING;
	}
	else if (0 == str_casecmp(name, ice_attr_ufrag))
		return ufrag_decode(ice, value);
	else if (0 == str_casecmp(name, ice_attr_pwd))
		return pwd_decode(ice, value);

	return 0;
}


/**
 * Decode SDP media attributes
 *
 * @param icem  ICE Media object
 * @param name  Name of the SDP attribute
 * @param value Value of the SDP attribute (optional)
 *
 * @return 0 if success, otherwise errorcode
 */
int icem_sdp_decode(struct icem *icem, const char *name, const char *value)
{
	if (!icem)
		return EINVAL;

	if (0 == str_casecmp(name, ice_attr_cand))
		return cand_decode(icem, value);
	else if (0 == str_casecmp(name, ice_attr_mismatch))
		icem->mismatch = true;
	else if (0 == str_casecmp(name, ice_attr_ufrag))
		return media_ufrag_decode(icem, value);
	else if (0 == str_casecmp(name, ice_attr_pwd))
		return media_pwd_decode(icem, value);

	return 0;
}


static const char *ice_tcptype_name(enum ice_tcptype tcptype)
{
	switch (tcptype) {

	case ICE_TCP_ACTIVE:  return "active";
	case ICE_TCP_PASSIVE: return "passive";
	case ICE_TCP_SO:      return "so";
	default: return "???";
	}
}


static enum ice_tcptype ice_tcptype_resolve(const struct pl *pl)
{
	if (0 == pl_strcasecmp(pl, "active"))  return ICE_TCP_ACTIVE;
	if (0 == pl_strcasecmp(pl, "passive")) return ICE_TCP_PASSIVE;
	if (0 == pl_strcasecmp(pl, "so"))      return ICE_TCP_SO;

	return (enum ice_tcptype)-1;
}


int ice_cand_attr_encode(struct re_printf *pf,
			 const struct ice_cand_attr *cand)
{
	int err = 0;

	if (!cand)
		return 0;

	err |= re_hprintf(pf, "%s %u %s %u %j %u typ %s",
			  cand->foundation, cand->compid,
			  net_proto2name(cand->proto), cand->prio,
			  &cand->addr, sa_port(&cand->addr),
			  ice_cand_type2name(cand->type));

	if (sa_isset(&cand->rel_addr, SA_ADDR))
		err |= re_hprintf(pf, " raddr %j", &cand->rel_addr);

	if (sa_isset(&cand->rel_addr, SA_PORT))
		err |= re_hprintf(pf, " rport %u", sa_port(&cand->rel_addr));

	if (cand->proto == IPPROTO_TCP) {
		err |= re_hprintf(pf, " tcptype %s",
				  ice_tcptype_name(cand->tcptype));
	}

	return err;
}


int ice_cand_attr_decode(struct ice_cand_attr *cand, const char *val)
{
	struct pl pl_fnd, pl_compid, pl_transp, pl_prio, pl_addr, pl_port;
	struct pl pl_type, pl_raddr, pl_rport, pl_opt = PL_INIT;
	size_t len;
	char type[8];
	int err;

	if (!cand || !val)
		return EINVAL;

	memset(cand, 0, sizeof(*cand));

	len = str_len(val);

	err = re_regex(val, len,
		       "[^ ]+ [0-9]+ [a-z]+ [0-9]+ [^ ]+ [0-9]+ typ [a-z]+"
		       "[^]*",
		       &pl_fnd, &pl_compid, &pl_transp, &pl_prio,
		       &pl_addr, &pl_port, &pl_type, &pl_opt);
	if (err)
		return err;

	(void)pl_strcpy(&pl_fnd, cand->foundation, sizeof(cand->foundation));

	if (0 == pl_strcasecmp(&pl_transp, "UDP"))
		cand->proto = IPPROTO_UDP;
	else if (0 == pl_strcasecmp(&pl_transp, "TCP"))
		cand->proto = IPPROTO_TCP;
	else
		cand->proto = 0;

	err = sa_set(&cand->addr, &pl_addr, pl_u32(&pl_port));
	if (err)
		return err;

	cand->compid = pl_u32(&pl_compid);
	cand->prio   = pl_u32(&pl_prio);

	(void)pl_strcpy(&pl_type, type, sizeof(type));

	cand->type = ice_cand_name2type(type);

	/* optional */

	if (0 == re_regex(pl_opt.p, pl_opt.l, "raddr [^ ]+ rport [0-9]+",
			  &pl_raddr, &pl_rport)) {

		err = sa_set(&cand->rel_addr, &pl_raddr, pl_u32(&pl_rport));
		if (err)
			return err;
	}

	if (cand->proto == IPPROTO_TCP) {

		struct pl tcptype;

		err = re_regex(pl_opt.p, pl_opt.l, "tcptype [^ ]+",
			       &tcptype);
		if (err)
			return err;

		cand->tcptype = ice_tcptype_resolve(&tcptype);
	}

	return 0;
}
