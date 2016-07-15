/**
 * @file dns/rr.c  DNS Resource Records
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_net.h>
#include <re_sa.h>
#include <re_dns.h>


static void rr_destructor(void *data)
{
	struct dnsrr *rr = data;

	mem_deref(rr->name);

	switch (rr->type) {

	case DNS_TYPE_NS:
		mem_deref(rr->rdata.ns.nsdname);
		break;

	case DNS_TYPE_CNAME:
		mem_deref(rr->rdata.cname.cname);
		break;

	case DNS_TYPE_SOA:
		mem_deref(rr->rdata.soa.mname);
		mem_deref(rr->rdata.soa.rname);
		break;

	case DNS_TYPE_PTR:
		mem_deref(rr->rdata.ptr.ptrdname);
		break;

	case DNS_TYPE_MX:
		mem_deref(rr->rdata.mx.exchange);
		break;

	case DNS_TYPE_SRV:
		mem_deref(rr->rdata.srv.target);
		break;

	case DNS_TYPE_NAPTR:
		mem_deref(rr->rdata.naptr.flags);
		mem_deref(rr->rdata.naptr.services);
		mem_deref(rr->rdata.naptr.regexp);
		mem_deref(rr->rdata.naptr.replace);
		break;
	}
}


/**
 * Allocate a new DNS Resource Record (RR)
 *
 * @return Newly allocated Resource Record, or NULL if no memory
 */
struct dnsrr *dns_rr_alloc(void)
{
	return mem_zalloc(sizeof(struct dnsrr), rr_destructor);
}


/**
 * Encode a DNS Resource Record
 *
 * @param mb       Memory buffer to encode into
 * @param rr       DNS Resource Record
 * @param ttl_offs TTL Offset
 * @param ht_dname Domain name hash-table
 * @param start    Start position
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_rr_encode(struct mbuf *mb, const struct dnsrr *rr, int64_t ttl_offs,
		  struct hash *ht_dname, size_t start)
{
	uint32_t ttl;
	uint16_t len;
	size_t start_rdata;
	int err = 0;

	if (!mb || !rr)
		return EINVAL;

	ttl = (uint32_t)((rr->ttl > ttl_offs) ? (rr->ttl - ttl_offs) : 0);

	err |= dns_dname_encode(mb, rr->name, ht_dname, start, true);
	err |= mbuf_write_u16(mb, htons(rr->type));
	err |= mbuf_write_u16(mb, htons(rr->dnsclass));
	err |= mbuf_write_u32(mb, htonl(ttl));
	err |= mbuf_write_u16(mb, htons(rr->rdlen));

	start_rdata = mb->pos;

	switch (rr->type) {

	case DNS_TYPE_A:
		err |= mbuf_write_u32(mb, htonl(rr->rdata.a.addr));
		break;

	case DNS_TYPE_NS:
		err |= dns_dname_encode(mb, rr->rdata.ns.nsdname,
					ht_dname, start, true);
		break;

	case DNS_TYPE_CNAME:
		err |= dns_dname_encode(mb, rr->rdata.cname.cname,
					ht_dname, start, true);
		break;

	case DNS_TYPE_SOA:
		err |= dns_dname_encode(mb, rr->rdata.soa.mname,
					ht_dname, start, true);
		err |= dns_dname_encode(mb, rr->rdata.soa.rname,
					ht_dname, start, true);
		err |= mbuf_write_u32(mb, htonl(rr->rdata.soa.serial));
		err |= mbuf_write_u32(mb, htonl(rr->rdata.soa.refresh));
		err |= mbuf_write_u32(mb, htonl(rr->rdata.soa.retry));
		err |= mbuf_write_u32(mb, htonl(rr->rdata.soa.expire));
		err |= mbuf_write_u32(mb, htonl(rr->rdata.soa.ttlmin));
		break;

	case DNS_TYPE_PTR:
		err |= dns_dname_encode(mb, rr->rdata.ptr.ptrdname,
					ht_dname, start, true);
		break;

	case DNS_TYPE_MX:
		err |= mbuf_write_u16(mb, htons(rr->rdata.mx.pref));
		err |= dns_dname_encode(mb, rr->rdata.mx.exchange,
					ht_dname, start, true);
		break;

	case DNS_TYPE_AAAA:
		err |= mbuf_write_mem(mb, rr->rdata.aaaa.addr, 16);
		break;

	case DNS_TYPE_SRV:
		err |= mbuf_write_u16(mb, htons(rr->rdata.srv.pri));
		err |= mbuf_write_u16(mb, htons(rr->rdata.srv.weight));
		err |= mbuf_write_u16(mb, htons(rr->rdata.srv.port));
		err |= dns_dname_encode(mb, rr->rdata.srv.target,
					ht_dname, start, false);
		break;

	case DNS_TYPE_NAPTR:
		err |= mbuf_write_u16(mb, htons(rr->rdata.naptr.order));
		err |= mbuf_write_u16(mb, htons(rr->rdata.naptr.pref));
		err |= dns_cstr_encode(mb, rr->rdata.naptr.flags);
		err |= dns_cstr_encode(mb, rr->rdata.naptr.services);
		err |= dns_cstr_encode(mb, rr->rdata.naptr.regexp);
		err |= dns_dname_encode(mb, rr->rdata.naptr.replace,
					ht_dname, start, false);
		break;

	default:
		err = EINVAL;
		break;
	}

	len = mb->pos - start_rdata;
	mb->pos = start_rdata - 2;
	err |= mbuf_write_u16(mb, htons(len));
	mb->pos += len;

	return err;
}


/**
 * Decode a DNS Resource Record (RR) from a memory buffer
 *
 * @param mb    Memory buffer to decode from
 * @param rr    Pointer to allocated Resource Record
 * @param start Start position
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_rr_decode(struct mbuf *mb, struct dnsrr **rr, size_t start)
{
	int err = 0;
	struct dnsrr *lrr;

	if (!mb || !rr)
		return EINVAL;

	lrr = dns_rr_alloc();
	if (!lrr)
		return ENOMEM;

	err = dns_dname_decode(mb, &lrr->name, start);
	if (err)
		goto error;

	if (mbuf_get_left(mb) < 10)
		goto fmerr;

	lrr->type     = ntohs(mbuf_read_u16(mb));
	lrr->dnsclass = ntohs(mbuf_read_u16(mb));
	lrr->ttl      = ntohl(mbuf_read_u32(mb));
	lrr->rdlen    = ntohs(mbuf_read_u16(mb));

	if (mbuf_get_left(mb) < lrr->rdlen)
		goto fmerr;

	switch (lrr->type) {

	case DNS_TYPE_A:
		if (lrr->rdlen != 4)
			goto fmerr;

		lrr->rdata.a.addr = ntohl(mbuf_read_u32(mb));
		break;

	case DNS_TYPE_NS:
		err = dns_dname_decode(mb, &lrr->rdata.ns.nsdname, start);
		if (err)
			goto error;

		break;

	case DNS_TYPE_CNAME:
		err = dns_dname_decode(mb, &lrr->rdata.cname.cname, start);
		if (err)
			goto error;

		break;

	case DNS_TYPE_SOA:
		err = dns_dname_decode(mb, &lrr->rdata.soa.mname, start);
		if (err)
			goto error;

		err = dns_dname_decode(mb, &lrr->rdata.soa.rname, start);
		if (err)
			goto error;

		if (mbuf_get_left(mb) < 20)
			goto fmerr;

		lrr->rdata.soa.serial  = ntohl(mbuf_read_u32(mb));
		lrr->rdata.soa.refresh = ntohl(mbuf_read_u32(mb));
		lrr->rdata.soa.retry   = ntohl(mbuf_read_u32(mb));
		lrr->rdata.soa.expire  = ntohl(mbuf_read_u32(mb));
		lrr->rdata.soa.ttlmin  = ntohl(mbuf_read_u32(mb));
		break;

	case DNS_TYPE_PTR:
		err = dns_dname_decode(mb, &lrr->rdata.ptr.ptrdname, start);
		if (err)
			goto error;

		break;

	case DNS_TYPE_MX:
		if (mbuf_get_left(mb) < 2)
			goto fmerr;

		lrr->rdata.mx.pref = ntohs(mbuf_read_u16(mb));

		err = dns_dname_decode(mb, &lrr->rdata.mx.exchange, start);
		if (err)
			goto error;

		break;

	case DNS_TYPE_AAAA:
		if (lrr->rdlen != 16)
			goto fmerr;

		err = mbuf_read_mem(mb, lrr->rdata.aaaa.addr, 16);
		if (err)
			goto error;
		break;

	case DNS_TYPE_SRV:
		if (mbuf_get_left(mb) < 6)
			goto fmerr;

		lrr->rdata.srv.pri    = ntohs(mbuf_read_u16(mb));
		lrr->rdata.srv.weight = ntohs(mbuf_read_u16(mb));
		lrr->rdata.srv.port   = ntohs(mbuf_read_u16(mb));

		err = dns_dname_decode(mb, &lrr->rdata.srv.target, start);
		if (err)
			goto error;

		break;

	case DNS_TYPE_NAPTR:
		if (mbuf_get_left(mb) < 4)
			goto fmerr;

		lrr->rdata.naptr.order = ntohs(mbuf_read_u16(mb));
		lrr->rdata.naptr.pref  = ntohs(mbuf_read_u16(mb));

		err = dns_cstr_decode(mb, &lrr->rdata.naptr.flags);
		if (err)
			goto error;

		err = dns_cstr_decode(mb, &lrr->rdata.naptr.services);
		if (err)
			goto error;

		err = dns_cstr_decode(mb, &lrr->rdata.naptr.regexp);
		if (err)
			goto error;

		err = dns_dname_decode(mb, &lrr->rdata.naptr.replace, start);
		if (err)
			goto error;

		break;

	default:
		mb->pos += lrr->rdlen;
		break;
	}

	*rr = lrr;

	return 0;

 fmerr:
	err = EINVAL;
 error:
	mem_deref(lrr);

	return err;
}


/**
 * Compare two DNS Resource Records
 *
 * @param rr1   First Resource Record
 * @param rr2   Second Resource Record
 * @param rdata If true, also compares Resource Record data
 *
 * @return True if match, false if not match
 */
bool dns_rr_cmp(const struct dnsrr *rr1, const struct dnsrr *rr2, bool rdata)
{
	if (!rr1 || !rr2)
		return false;

	if (rr1 == rr2)
		return true;

	if (rr1->type != rr2->type)
		return false;

	if (rr1->dnsclass != rr2->dnsclass)
		return false;

	if (str_casecmp(rr1->name, rr2->name))
		return false;

	if (!rdata)
		return true;

	switch (rr1->type) {

	case DNS_TYPE_A:
		if (rr1->rdata.a.addr != rr2->rdata.a.addr)
			return false;

		break;

	case DNS_TYPE_NS:
		if (str_casecmp(rr1->rdata.ns.nsdname, rr2->rdata.ns.nsdname))
			return false;

		break;

	case DNS_TYPE_CNAME:
		if (str_casecmp(rr1->rdata.cname.cname,
				rr2->rdata.cname.cname))
			return false;

		break;

	case DNS_TYPE_SOA:
		if (str_casecmp(rr1->rdata.soa.mname, rr2->rdata.soa.mname))
			return false;

		if (str_casecmp(rr1->rdata.soa.rname, rr2->rdata.soa.rname))
			return false;

		if (rr1->rdata.soa.serial != rr2->rdata.soa.serial)
			return false;

		if (rr1->rdata.soa.refresh != rr2->rdata.soa.refresh)
			return false;

		if (rr1->rdata.soa.retry != rr2->rdata.soa.retry)
			return false;

		if (rr1->rdata.soa.expire != rr2->rdata.soa.expire)
			return false;

		if (rr1->rdata.soa.ttlmin != rr2->rdata.soa.ttlmin)
			return false;

		break;

	case DNS_TYPE_PTR:
		if (str_casecmp(rr1->rdata.ptr.ptrdname,
				rr2->rdata.ptr.ptrdname))
			return false;

		break;

	case DNS_TYPE_MX:
		if (rr1->rdata.mx.pref != rr2->rdata.mx.pref)
			return false;

		if (str_casecmp(rr1->rdata.mx.exchange,
				rr2->rdata.mx.exchange))
			return false;

		break;

	case DNS_TYPE_AAAA:
		if (memcmp(rr1->rdata.aaaa.addr, rr2->rdata.aaaa.addr, 16))
			return false;

		break;

	case DNS_TYPE_SRV:
		if (rr1->rdata.srv.pri != rr2->rdata.srv.pri)
			return false;

		if (rr1->rdata.srv.weight != rr2->rdata.srv.weight)
			return false;

		if (rr1->rdata.srv.port != rr2->rdata.srv.port)
			return false;

		if (str_casecmp(rr1->rdata.srv.target, rr2->rdata.srv.target))
			return false;

		break;

	case DNS_TYPE_NAPTR:
		if (rr1->rdata.naptr.order != rr2->rdata.naptr.order)
			return false;

		if (rr1->rdata.naptr.pref != rr2->rdata.naptr.pref)
			return false;

		/* todo check case sensitiveness */
		if (str_casecmp(rr1->rdata.naptr.flags,
				rr2->rdata.naptr.flags))
			return false;

		/* todo check case sensitiveness */
		if (str_casecmp(rr1->rdata.naptr.services,
				rr2->rdata.naptr.services))
			return false;

		/* todo check case sensitiveness */
		if (str_casecmp(rr1->rdata.naptr.regexp,
				rr2->rdata.naptr.regexp))
			return false;

		/* todo check case sensitiveness */
		if (str_casecmp(rr1->rdata.naptr.replace,
				rr2->rdata.naptr.replace))
			return false;

		break;

	default:
		return false;
	}

	return true;
}


/**
 * Get the DNS Resource Record (RR) name
 *
 * @param type DNS Resource Record type
 *
 * @return DNS Resource Record name
 */
const char *dns_rr_typename(uint16_t type)
{
	switch (type) {

	case DNS_TYPE_A:     return "A";
	case DNS_TYPE_NS:    return "NS";
	case DNS_TYPE_CNAME: return "CNAME";
	case DNS_TYPE_SOA:   return "SOA";
	case DNS_TYPE_PTR:   return "PTR";
	case DNS_TYPE_MX:    return "MX";
	case DNS_TYPE_AAAA:  return "AAAA";
	case DNS_TYPE_SRV:   return "SRV";
	case DNS_TYPE_NAPTR: return "NAPTR";
	case DNS_QTYPE_IXFR: return "IXFR";
	case DNS_QTYPE_AXFR: return "AXFR";
	case DNS_QTYPE_ANY:  return "ANY";
	default:             return "??";
	}
}


/**
 * Get the DNS Resource Record (RR) class name
 *
 * @param dnsclass DNS Class
 *
 * @return DNS Class name
 */
const char *dns_rr_classname(uint16_t dnsclass)
{
	switch (dnsclass) {

	case DNS_CLASS_IN:   return "IN";
	case DNS_QCLASS_ANY: return "ANY";
	default:             return "??";
	}
}


/**
 * Print a DNS Resource Record
 *
 * @param pf Print function
 * @param rr DNS Resource Record
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_rr_print(struct re_printf *pf, const struct dnsrr *rr)
{
	static const size_t w = 24;
	struct sa sa;
	size_t n, l;
	int err;

	if (!pf || !rr)
		return EINVAL;

	l = str_len(rr->name);
	n = (w > l) ? w - l : 0;

	err = re_hprintf(pf, "%s.", rr->name);
	while (n--)
		err |= pf->vph(" ", 1, pf->arg);

	err |= re_hprintf(pf, " %10lld %-4s %-7s ",
			  rr->ttl,
			  dns_rr_classname(rr->dnsclass),
			  dns_rr_typename(rr->type));

	switch (rr->type) {

	case DNS_TYPE_A:
		sa_set_in(&sa, rr->rdata.a.addr, 0);
		err |= re_hprintf(pf, "%j", &sa);
		break;

	case DNS_TYPE_NS:
		err |= re_hprintf(pf, "%s.", rr->rdata.ns.nsdname);
		break;

	case DNS_TYPE_CNAME:
		err |= re_hprintf(pf, "%s.", rr->rdata.cname.cname);
		break;

	case DNS_TYPE_SOA:
		err |= re_hprintf(pf, "%s. %s. %u %u %u %u %u",
				  rr->rdata.soa.mname,
				  rr->rdata.soa.rname,
				  rr->rdata.soa.serial,
				  rr->rdata.soa.refresh,
				  rr->rdata.soa.retry,
				  rr->rdata.soa.expire,
				  rr->rdata.soa.ttlmin);
		break;

	case DNS_TYPE_PTR:
		err |= re_hprintf(pf, "%s.", rr->rdata.ptr.ptrdname);
		break;

	case DNS_TYPE_MX:
		err |= re_hprintf(pf, "%3u %s.", rr->rdata.mx.pref,
				  rr->rdata.mx.exchange);
		break;

	case DNS_TYPE_AAAA:
		sa_set_in6(&sa, rr->rdata.aaaa.addr, 0);
		err |= re_hprintf(pf, "%j", &sa);
		break;

	case DNS_TYPE_SRV:
		err |= re_hprintf(pf, "%3u %3u %u %s.",
				  rr->rdata.srv.pri,
				  rr->rdata.srv.weight,
				  rr->rdata.srv.port,
				  rr->rdata.srv.target);
		break;

	case DNS_TYPE_NAPTR:
		err |= re_hprintf(pf, "%3u %3u \"%s\" \"%s\" \"%s\" %s.",
				  rr->rdata.naptr.order,
				  rr->rdata.naptr.pref,
				  rr->rdata.naptr.flags,
				  rr->rdata.naptr.services,
				  rr->rdata.naptr.regexp,
				  rr->rdata.naptr.replace);
		break;

	default:
		err |= re_hprintf(pf, "?");
		break;
	}

	return err;
}
