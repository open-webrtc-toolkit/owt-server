/**
 * @file rrlist.c  DNS Resource Records list
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_list.h>
#include <re_mbuf.h>
#include <re_fmt.h>
#include <re_dns.h>


enum {
	CNAME_RECURSE_MAX = 16,
};


static bool std_sort_handler(struct le *le1, struct le *le2, void *arg)
{
	struct dnsrr *rr1 = le1->data;
	struct dnsrr *rr2 = le2->data;
	const uint16_t type = *(uint16_t *)arg;

	if (type != rr1->type)
		return type != rr2->type;

	if (type != rr2->type)
		return true;

	switch (type) {

	case DNS_TYPE_MX:
		return rr1->rdata.mx.pref <= rr2->rdata.mx.pref;

	case DNS_TYPE_SRV:
		if (rr1->rdata.srv.pri == rr2->rdata.srv.pri) {

			if (rr1->rdata.srv.weight)
				return 0 != rr2->rdata.srv.weight;

			return true;
		}

		return rr1->rdata.srv.pri < rr2->rdata.srv.pri;

	case DNS_TYPE_NAPTR:
		if (rr1->rdata.naptr.order == rr2->rdata.naptr.order)
			return rr1->rdata.naptr.pref <= rr2->rdata.naptr.pref;

		return rr1->rdata.naptr.order < rr2->rdata.naptr.order;

	default:
		break;
	}

	return true;
}


/**
 * Sort a list of DNS Resource Records
 *
 * @param rrl  DNS Resource Record list
 * @param type DNS Record type
 */
void dns_rrlist_sort(struct list *rrl, uint16_t type)
{
	list_sort(rrl, std_sort_handler, &type);
	/* todo add SRV postprocessing for weighted load balancing. */
}


static struct dnsrr *rrlist_apply(struct list *rrl, const char *name,
				  uint16_t type1, uint16_t type2,
				  uint16_t dnsclass,
				  bool recurse, uint32_t depth,
				  dns_rrlist_h *rrlh, void *arg)
{
	struct le *le = list_head(rrl);

	if (depth > CNAME_RECURSE_MAX)
		return NULL;

	while (le) {

		struct dnsrr *rr = le->data;

		le = le->next;

		if (name && str_casecmp(name, rr->name))
			continue;

		if (type1 != DNS_QTYPE_ANY && type2 != DNS_QTYPE_ANY &&
		    rr->type != type1 && rr->type != type2 &&
		    (rr->type != DNS_TYPE_CNAME || !recurse))
			continue;

		if (dnsclass != DNS_QCLASS_ANY && rr->dnsclass != dnsclass)
			continue;

		if (!rrlh || rrlh(rr, arg))
			return rr;

		if (recurse &&
		    DNS_QTYPE_ANY != type1 && DNS_QTYPE_ANY != type2 &&
		    DNS_TYPE_CNAME != type1 && DNS_TYPE_CNAME != type2 &&
		    DNS_TYPE_CNAME == rr->type) {
			rr = rrlist_apply(rrl, rr->rdata.cname.cname, type1,
					  type2, dnsclass, recurse, ++depth,
					  rrlh, arg);
			if (rr)
				return rr;
		}
	}

	return NULL;
}


/**
 * Apply a function handler to a list of DNS Resource Records
 *
 * @param rrl      DNS Resource Record list
 * @param name     If set, filter on domain name
 * @param type     If not DNS_QTYPE_ANY, filter on record type
 * @param dnsclass If not DNS_QCLASS_ANY, filter on DNS class
 * @param recurse  Cname recursion
 * @param rrlh     Resource record handler
 * @param arg      Handler argument
 *
 * @return Matching Resource Record or NULL
 */
struct dnsrr *dns_rrlist_apply(struct list *rrl, const char *name,
			       uint16_t type, uint16_t dnsclass,
			       bool recurse, dns_rrlist_h *rrlh, void *arg)
{
	return rrlist_apply(rrl, name, type, type, dnsclass,
			    recurse, 0, rrlh, arg);
}


/**
 * Apply a function handler to a list of DNS Resource Records (two types)
 *
 * @param rrl      DNS Resource Record list
 * @param name     If set, filter on domain name
 * @param type1    If not DNS_QTYPE_ANY, filter on record type
 * @param type2    If not DNS_QTYPE_ANY, filter on record type
 * @param dnsclass If not DNS_QCLASS_ANY, filter on DNS class
 * @param recurse  Cname recursion
 * @param rrlh     Resource record handler
 * @param arg      Handler argument
 *
 * @return Matching Resource Record or NULL
 */
struct dnsrr *dns_rrlist_apply2(struct list *rrl, const char *name,
				uint16_t type1, uint16_t type2,
				uint16_t dnsclass, bool recurse,
				dns_rrlist_h *rrlh, void *arg)
{
	return rrlist_apply(rrl, name, type1, type2, dnsclass,
			    recurse, 0, rrlh, arg);
}


static bool find_handler(struct dnsrr *rr, void *arg)
{
	uint16_t type = *(uint16_t *)arg;

	return rr->type == type;
}


/**
 * Find a DNS Resource Record in a list
 *
 * @param rrl      Resource Record list
 * @param name     If set, filter on domain name
 * @param type     If not DNS_QTYPE_ANY, filter on record type
 * @param dnsclass If not DNS_QCLASS_ANY, filter on DNS class
 * @param recurse  Cname recursion
 *
 * @return Matching Resource Record or NULL
 */
struct dnsrr *dns_rrlist_find(struct list *rrl, const char *name,
			      uint16_t type, uint16_t dnsclass, bool recurse)
{
	return rrlist_apply(rrl, name, type, type, dnsclass,
			    recurse, 0, find_handler, &type);
}
