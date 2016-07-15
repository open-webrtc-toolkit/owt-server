/**
 * @file dnsdisc.c  DNS Discovery of a STUN Server
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_dns.h>
#include <re_stun.h>


#define DEBUG_MODULE "dnsdisc"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/** DNS Query */
struct stun_dns {
	char domain[256];      /**< Cached domain name      */
	stun_dns_h *dnsh;      /**< DNS Response handler    */
	void *arg;             /**< Handler argument        */
	struct sa srv;         /**< Resolved server address */
	struct dnsc *dnsc;     /**< DNS Client              */
	struct dns_query *dq;  /**< Current DNS query       */
	int af;                /**< Preferred Address family*/
	uint16_t port;         /**< Default Port            */
};

const char *stun_proto_udp = "udp";   /**< UDP Protocol  */
const char *stun_proto_tcp = "tcp";   /**< TCP Protocol  */

const char *stun_usage_binding   = "stun";  /**< Binding usage */
const char *stuns_usage_binding  = "stuns"; /**< Binding usage TLS */
const char *stun_usage_relay     = "turn";
const char *stuns_usage_relay    = "turns";
const char *stun_usage_behavior  = "stun-behavior";
const char *stuns_usage_behavior = "stun-behaviors";


static void resolved(const struct stun_dns *dns, int err)
{
	stun_dns_h *dnsh = dns->dnsh;
	void *dnsh_arg = dns->arg;

	DEBUG_INFO("resolved: %J (%m)\n", &dns->srv, err);

	dnsh(err, &dns->srv, dnsh_arg);
}


static void a_handler(int err, const struct dnshdr *hdr, struct list *ansl,
		      struct list *authl, struct list *addl, void *arg)
{
	struct stun_dns *dns = arg;
	struct dnsrr *rr;

	(void)hdr;
	(void)authl;
	(void)addl;

	/* Find A answers */
	rr = dns_rrlist_find(ansl, NULL, DNS_TYPE_A, DNS_CLASS_IN, false);
	if (!rr) {
		err = err ? err : EDESTADDRREQ;
		goto out;
	}

	sa_set_in(&dns->srv, rr->rdata.a.addr, sa_port(&dns->srv));

	DEBUG_INFO("A answer: %j\n", &dns->srv);

 out:
	resolved(dns, err);
}


#ifdef HAVE_INET6
static void aaaa_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			 struct list *authl, struct list *addl, void *arg)
{
	struct stun_dns *dns = arg;
	struct dnsrr *rr;

	(void)hdr;
	(void)authl;
	(void)addl;

	/* Find A answers */
	rr = dns_rrlist_find(ansl, NULL, DNS_TYPE_AAAA, DNS_CLASS_IN, false);
	if (!rr) {
		err = err ? err : EDESTADDRREQ;
		goto out;
	}

	sa_set_in6(&dns->srv, rr->rdata.aaaa.addr, sa_port(&dns->srv));

	DEBUG_INFO("AAAA answer: %j\n", &dns->srv);

 out:
	resolved(dns, err);
}
#endif


static int a_or_aaaa_query(struct stun_dns *dns, const char *name)
{
	dns->dq = mem_deref(dns->dq);

	switch (dns->af) {

	case AF_INET:
		return dnsc_query(&dns->dq, dns->dnsc, name, DNS_TYPE_A,
				  DNS_CLASS_IN, true, a_handler, dns);

#ifdef HAVE_INET6
	case AF_INET6:
		return dnsc_query(&dns->dq, dns->dnsc, name, DNS_TYPE_AAAA,
				  DNS_CLASS_IN, true, aaaa_handler, dns);
#endif

	default:
		return EAFNOSUPPORT;
	}
}


static void srv_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			struct list *authl, struct list *addl, void *arg)
{
	struct stun_dns *dns = arg;
	struct dnsrr *rr, *arr;

	(void)hdr;
	(void)authl;

	dns_rrlist_sort(ansl, DNS_TYPE_SRV);

	/* Find SRV answers */
	rr = dns_rrlist_find(ansl, NULL, DNS_TYPE_SRV, DNS_CLASS_IN, false);
	if (!rr) {
		DEBUG_INFO("no SRV entry, trying A lookup on \"%s\"\n",
			   dns->domain);

		sa_set_in(&dns->srv, 0, dns->port);

		err = a_or_aaaa_query(dns, dns->domain);
		if (err)
			goto out;

		return;
	}

	DEBUG_INFO("SRV answer: %s:%u\n", rr->rdata.srv.target,
		   rr->rdata.srv.port);

	/* Look for Additional information */
	switch (dns->af) {

	case AF_INET:
		arr = dns_rrlist_find(addl, rr->rdata.srv.target,
				      DNS_TYPE_A, DNS_CLASS_IN, true);
		if (arr) {
			sa_set_in(&dns->srv, arr->rdata.a.addr,
				  rr->rdata.srv.port);
			DEBUG_INFO("additional A: %j\n", &dns->srv);
			goto out;
		}
		break;

#ifdef HAVE_INET6
	case AF_INET6:
		arr = dns_rrlist_find(addl, rr->rdata.srv.target,
				      DNS_TYPE_AAAA, DNS_CLASS_IN, true);
		if (arr) {
			sa_set_in6(&dns->srv, arr->rdata.aaaa.addr,
				  rr->rdata.srv.port);
			DEBUG_INFO("additional AAAA: %j\n", &dns->srv);
			goto out;
		}
		break;
#endif
	}

	sa_set_in(&dns->srv, 0, rr->rdata.srv.port);

	err = a_or_aaaa_query(dns, rr->rdata.srv.target);
	if (err) {
		DEBUG_WARNING("SRV: A lookup failed (%m)\n", err);
		goto out;
	}

	DEBUG_INFO("SRV handler: doing A/AAAA lookup..\n");

	return;

 out:
	resolved(dns, err);
}


static void dnsdisc_destructor(void *data)
{
	struct stun_dns *dns = data;

	mem_deref(dns->dq);
}


/**
 * Do a DNS Discovery of a STUN Server
 *
 * @param dnsp    Pointer to allocated DNS Discovery object
 * @param dnsc    DNS Client
 * @param service Name of service to discover (e.g. "stun")
 * @param proto   Transport protocol (e.g. "udp")
 * @param af      Preferred Address Family
 * @param domain  Domain name or IP address of STUN server
 * @param port    Port number (if 0 do SRV lookup)
 * @param dnsh    DNS Response handler
 * @param arg     Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_server_discover(struct stun_dns **dnsp, struct dnsc *dnsc,
			 const char *service, const char *proto,
			 int af, const char *domain, uint16_t port,
			 stun_dns_h *dnsh, void *arg)
{
	struct stun_dns *dns;
	int err;

	if (!dnsp || !service || !proto || !domain || !domain[0] || !dnsh)
		return EINVAL;

	dns = mem_zalloc(sizeof(*dns), dnsdisc_destructor);
	if (!dns)
		return ENOMEM;

	dns->port = service[strlen(service)-1] == 's' ? STUNS_PORT : STUN_PORT;
	dns->dnsh = dnsh;
	dns->arg  = arg;
	dns->dnsc = dnsc;
	dns->af   = af;

	/* Numeric IP address - no lookup */
	if (0 == sa_set_str(&dns->srv, domain, port ? port : dns->port)) {

		DEBUG_INFO("IP (%s)\n", domain);

		resolved(dns, 0);
		err = 0;
		goto out; /* free now */
	}
	/* Port specified - use AAAA or A lookup */
	else if (port) {
		sa_set_in(&dns->srv, 0, port);
		DEBUG_INFO("resolving A query: (%s)\n", domain);

		err = a_or_aaaa_query(dns, domain);
		if (err) {
			DEBUG_WARNING("%s: A/AAAA lookup failed (%m)\n",
				      domain, err);
			goto out;
		}
	}
	/* SRV lookup */
	else {
		char q[256];
		str_ncpy(dns->domain, domain, sizeof(dns->domain));
		(void)re_snprintf(q, sizeof(q), "_%s._%s.%s", service, proto,
				  domain);
		DEBUG_INFO("resolving SRV query: (%s)\n", q);
		err = dnsc_query(&dns->dq, dnsc, q, DNS_TYPE_SRV, DNS_CLASS_IN,
				 true, srv_handler, dns);
		if (err) {
			DEBUG_WARNING("%s: SRV lookup failed (%m)\n", q, err);
			goto out;
		}
	}

	*dnsp = dns;

	return 0;

 out:
	mem_deref(dns);
	return err;
}
