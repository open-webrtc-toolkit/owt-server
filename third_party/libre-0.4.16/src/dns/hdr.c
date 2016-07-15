/**
 * @file dns/hdr.c  DNS header encoding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_mbuf.h>
#include <re_net.h>
#include <re_dns.h>


enum {
	QUERY_RESPONSE      = 15,
	OPCODE              = 11,
	AUTH_ANSWER         = 10,
	TRUNCATED           =  9,
	RECURSION_DESIRED   =  8,
	RECURSION_AVAILABLE =  7,
	ZERO                =  4
};


/**
 * Encode a DNS header
 *
 * @param mb  Memory buffer to encode header into
 * @param hdr DNS header
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_hdr_encode(struct mbuf *mb, const struct dnshdr *hdr)
{
	uint16_t flags = 0;
	int err = 0;

	if (!mb || !hdr)
		return EINVAL;

	flags |= hdr->qr     <<QUERY_RESPONSE;
	flags |= hdr->opcode <<OPCODE;
	flags |= hdr->aa     <<AUTH_ANSWER;
	flags |= hdr->tc     <<TRUNCATED;
	flags |= hdr->rd     <<RECURSION_DESIRED;
	flags |= hdr->ra     <<RECURSION_AVAILABLE;
	flags |= hdr->z      <<ZERO;
	flags |= hdr->rcode;

	err |= mbuf_write_u16(mb, htons(hdr->id));
	err |= mbuf_write_u16(mb, htons(flags));
	err |= mbuf_write_u16(mb, htons(hdr->nq));
	err |= mbuf_write_u16(mb, htons(hdr->nans));
	err |= mbuf_write_u16(mb, htons(hdr->nauth));
	err |= mbuf_write_u16(mb, htons(hdr->nadd));

	return err;
}


/**
 * Decode a DNS header from a memory buffer
 *
 * @param mb  Memory buffer to decode header from
 * @param hdr DNS header (output)
 *
 * @return 0 if success, otherwise errorcode
 */
int dns_hdr_decode(struct mbuf *mb, struct dnshdr *hdr)
{
	uint16_t flags = 0;

	if (!mb || !hdr || (mbuf_get_left(mb) < DNS_HEADER_SIZE))
		return EINVAL;

	hdr->id = ntohs(mbuf_read_u16(mb));
	flags   = ntohs(mbuf_read_u16(mb));

	hdr->qr     = 0x1 & (flags >> QUERY_RESPONSE);
	hdr->opcode = 0xf & (flags >> OPCODE);
	hdr->aa     = 0x1 & (flags >> AUTH_ANSWER);
	hdr->tc     = 0x1 & (flags >> TRUNCATED);
	hdr->rd     = 0x1 & (flags >> RECURSION_DESIRED);
	hdr->ra     = 0x1 & (flags >> RECURSION_AVAILABLE);
	hdr->z      = 0x7 & (flags >> ZERO);
	hdr->rcode  = 0xf & (flags >> 0);

	hdr->nq    = ntohs(mbuf_read_u16(mb));
	hdr->nans  = ntohs(mbuf_read_u16(mb));
	hdr->nauth = ntohs(mbuf_read_u16(mb));
	hdr->nadd  = ntohs(mbuf_read_u16(mb));

	return 0;
}


/**
 * Get the string of a DNS opcode
 *
 * @param opcode DNS opcode
 *
 * @return Opcode string
 */
const char *dns_hdr_opcodename(uint8_t opcode)
{
	switch (opcode) {

	case DNS_OPCODE_QUERY:  return "QUERY";
	case DNS_OPCODE_IQUERY: return "IQUERY";
	case DNS_OPCODE_STATUS: return "STATUS";
	case DNS_OPCODE_NOTIFY: return "NOTIFY";
	default:                return "??";
	}
}


/**
 * Get the string of a DNS response code
 *
 * @param rcode Response code
 *
 * @return Response code string
 */
const char *dns_hdr_rcodename(uint8_t rcode)
{
	switch (rcode) {

	case DNS_RCODE_OK:       return "OK";
	case DNS_RCODE_FMT_ERR:  return "Format Error";
	case DNS_RCODE_SRV_FAIL: return "Server Failure";
	case DNS_RCODE_NAME_ERR: return "Name Error";
	case DNS_RCODE_NOT_IMPL: return "Not Implemented";
	case DNS_RCODE_REFUSED:  return "Refused";
	case DNS_RCODE_NOT_AUTH: return "Server Not Authoritative for zone";
	default:                 return "??";
	}
}
