/**
 * @file stun/attr.c  STUN Attributes
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_sys.h>
#include <re_stun.h>
#include "stun.h"


static int str_decode(struct mbuf *mb, char **str, size_t len)
{
	if (mbuf_get_left(mb) < len)
		return EBADMSG;

	return mbuf_strdup(mb, str, len);
}


static void destructor(void *arg)
{
	struct stun_attr *attr = arg;

	switch (attr->type) {

	case STUN_ATTR_USERNAME:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE:
	case STUN_ATTR_SOFTWARE:
		mem_deref(attr->v.str);
		break;

	case STUN_ATTR_ERR_CODE:
		mem_deref(attr->v.err_code.reason);
		break;

	case STUN_ATTR_DATA:
	case STUN_ATTR_PADDING:
		mem_deref(attr->v.mb.buf);
		break;
	}

	list_unlink(&attr->le);
}


int stun_attr_encode(struct mbuf *mb, uint16_t type, const void *v,
		     const uint8_t *tid, uint8_t padding)
{
	const struct stun_change_req *ch_req = v;
	const struct stun_errcode *err_code = v;
	const struct stun_unknown_attr *ua = v;
	const unsigned int *num = v;
	const struct mbuf *mbd = v;
	size_t start, len;
	uint32_t i, n;
	int err = 0;

	if (!mb || !v)
		return EINVAL;

	mb->pos += 4;
	start = mb->pos;

	switch (type) {

	case STUN_ATTR_MAPPED_ADDR:
	case STUN_ATTR_ALT_SERVER:
	case STUN_ATTR_RESP_ORIGIN:
	case STUN_ATTR_OTHER_ADDR:
		tid = NULL;
		/*@fallthrough@*/
	case STUN_ATTR_XOR_PEER_ADDR:
	case STUN_ATTR_XOR_RELAY_ADDR:
	case STUN_ATTR_XOR_MAPPED_ADDR:
		err |= stun_addr_encode(mb, v, tid);
		break;

	case STUN_ATTR_CHANGE_REQ:
		n = (uint32_t)ch_req->ip << 2 | (uint32_t)ch_req->port << 1;
		err |= mbuf_write_u32(mb, htonl(n));
		break;

	case STUN_ATTR_USERNAME:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE:
	case STUN_ATTR_SOFTWARE:
		err |= mbuf_write_str(mb, v);
		break;

	case STUN_ATTR_MSG_INTEGRITY:
		err |= mbuf_write_mem(mb, v, 20);
		break;

	case STUN_ATTR_ERR_CODE:
		err |= mbuf_write_u16(mb, 0x00);
		err |= mbuf_write_u8(mb,  err_code->code/100);
		err |= mbuf_write_u8(mb,  err_code->code%100);
		err |= mbuf_write_str(mb, err_code->reason);
		break;

	case STUN_ATTR_UNKNOWN_ATTR:
		for (i=0; i<ua->typec; i++)
			err |= mbuf_write_u16(mb, htons(ua->typev[i]));
		break;

	case STUN_ATTR_CHANNEL_NUMBER:
	case STUN_ATTR_RESP_PORT:
		err |= mbuf_write_u16(mb, htons(*num));
		err |= mbuf_write_u16(mb, 0x0000);
		break;

	case STUN_ATTR_LIFETIME:
	case STUN_ATTR_PRIORITY:
	case STUN_ATTR_FINGERPRINT:
		err |= mbuf_write_u32(mb, htonl(*num));
		break;

	case STUN_ATTR_DATA:
	case STUN_ATTR_PADDING:
		if (mb == mbd) {
			mb->pos = mb->end;
			break;
		}
		err |= mbuf_write_mem(mb, mbuf_buf(mbd), mbuf_get_left(mbd));
		break;

	case STUN_ATTR_REQ_ADDR_FAMILY:
	case STUN_ATTR_REQ_TRANSPORT:
		err |= mbuf_write_u8(mb, *num);
		err |= mbuf_write_u8(mb, 0x00);
		err |= mbuf_write_u16(mb, 0x0000);
		break;

	case STUN_ATTR_EVEN_PORT:
		err |= mbuf_write_u8(mb, ((struct stun_even_port *)v)->r << 7);
		break;

	case STUN_ATTR_DONT_FRAGMENT:
	case STUN_ATTR_USE_CAND:
		 /* no value */
		break;

	case STUN_ATTR_RSV_TOKEN:
	case STUN_ATTR_CONTROLLED:
	case STUN_ATTR_CONTROLLING:
		err |= mbuf_write_u64(mb, sys_htonll(*(uint64_t *)v));
		break;

	default:
		err = EINVAL;
		break;
	}

	/* header */
	len = mb->pos - start;

	mb->pos = start - 4;
	err |= mbuf_write_u16(mb, htons(type));
	err |= mbuf_write_u16(mb, htons(len));
	mb->pos += len;

	/* padding */
	while ((mb->pos - start) & 0x03)
		err |= mbuf_write_u8(mb, padding);

	return err;
}


int stun_attr_decode(struct stun_attr **attrp, struct mbuf *mb,
		     const uint8_t *tid, struct stun_unknown_attr *ua)
{
	struct stun_attr *attr;
	size_t start, len;
	uint32_t i, n;
	int err = 0;

	if (!mb || !attrp)
		return EINVAL;

	if (mbuf_get_left(mb) < 4)
		return EBADMSG;

	attr = mem_zalloc(sizeof(*attr), destructor);
	if (!attr)
		return ENOMEM;

	attr->type = ntohs(mbuf_read_u16(mb));
	len = ntohs(mbuf_read_u16(mb));

	if (mbuf_get_left(mb) < len)
		goto badmsg;

	start = mb->pos;

	switch (attr->type) {

	case STUN_ATTR_MAPPED_ADDR:
	case STUN_ATTR_ALT_SERVER:
	case STUN_ATTR_RESP_ORIGIN:
	case STUN_ATTR_OTHER_ADDR:
		tid = NULL;
		/*@fallthrough@*/
	case STUN_ATTR_XOR_PEER_ADDR:
	case STUN_ATTR_XOR_RELAY_ADDR:
	case STUN_ATTR_XOR_MAPPED_ADDR:
		err = stun_addr_decode(mb, &attr->v.sa, tid);
		break;

	case STUN_ATTR_CHANGE_REQ:
		if (len != 4)
			goto badmsg;

		n = ntohl(mbuf_read_u32(mb));
		attr->v.change_req.ip   = (n >> 2) & 0x1;
		attr->v.change_req.port = (n >> 1) & 0x1;
		break;

	case STUN_ATTR_USERNAME:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE:
	case STUN_ATTR_SOFTWARE:
		err = str_decode(mb, &attr->v.str, len);
		break;

	case STUN_ATTR_MSG_INTEGRITY:
		if (len != 20)
			goto badmsg;

		err = mbuf_read_mem(mb, attr->v.msg_integrity, 20);
		break;

	case STUN_ATTR_ERR_CODE:
		if (len < 4)
			goto badmsg;

		mb->pos += 2;
		attr->v.err_code.code  = (mbuf_read_u8(mb) & 0x7) * 100;
		attr->v.err_code.code += mbuf_read_u8(mb);
		err = str_decode(mb, &attr->v.err_code.reason, len - 4);
		break;

	case STUN_ATTR_UNKNOWN_ATTR:
		for (i=0; i<len/2; i++) {
			uint16_t type = ntohs(mbuf_read_u16(mb));

			if (i >= ARRAY_SIZE(attr->v.unknown_attr.typev))
				continue;

			attr->v.unknown_attr.typev[i] = type;
			attr->v.unknown_attr.typec++;
		}
		break;

	case STUN_ATTR_CHANNEL_NUMBER:
	case STUN_ATTR_RESP_PORT:
		if (len < 2)
			goto badmsg;

	        attr->v.uint16 = ntohs(mbuf_read_u16(mb));
		break;

	case STUN_ATTR_LIFETIME:
	case STUN_ATTR_PRIORITY:
	case STUN_ATTR_FINGERPRINT:
		if (len != 4)
			goto badmsg;

	        attr->v.uint32 = ntohl(mbuf_read_u32(mb));
		break;

	case STUN_ATTR_DATA:
	case STUN_ATTR_PADDING:
		attr->v.mb.buf  = mem_ref(mb->buf);
		attr->v.mb.size = mb->size;
		attr->v.mb.pos  = mb->pos;
		attr->v.mb.end  = mb->pos + len;
		mb->pos += len;
		break;

	case STUN_ATTR_REQ_ADDR_FAMILY:
	case STUN_ATTR_REQ_TRANSPORT:
		if (len < 1)
			goto badmsg;

	        attr->v.uint8 = mbuf_read_u8(mb);
		break;

	case STUN_ATTR_EVEN_PORT:
		if (len < 1)
			goto badmsg;

		attr->v.even_port.r = (mbuf_read_u8(mb) >> 7) & 0x1;
		break;

	case STUN_ATTR_DONT_FRAGMENT:
	case STUN_ATTR_USE_CAND:
		if (len > 0)
			goto badmsg;

		/* no value */
		break;

	case STUN_ATTR_RSV_TOKEN:
	case STUN_ATTR_CONTROLLING:
	case STUN_ATTR_CONTROLLED:
		if (len != 8)
			goto badmsg;

	        attr->v.uint64 = sys_ntohll(mbuf_read_u64(mb));
		break;

	default:
		mb->pos += len;

		if (attr->type >= 0x8000)
			break;

		if (ua && ua->typec < ARRAY_SIZE(ua->typev))
			ua->typev[ua->typec++] = attr->type;
		break;
	}

	if (err)
		goto error;

	/* padding */
	while (((mb->pos - start) & 0x03) && mbuf_get_left(mb))
		++mb->pos;

	*attrp = attr;

	return 0;

 badmsg:
	err = EBADMSG;
 error:
	mem_deref(attr);

	return err;
}


/**
 * Get the name of a STUN attribute
 *
 * @param type STUN attribute type
 *
 * @return String with attribute name
 */
const char *stun_attr_name(uint16_t type)
{
	switch (type) {

	case STUN_ATTR_MAPPED_ADDR:       return "MAPPED-ADDRESS";
	case STUN_ATTR_CHANGE_REQ:        return "CHANGE-REQUEST";
	case STUN_ATTR_USERNAME:          return "USERNAME";
	case STUN_ATTR_MSG_INTEGRITY:     return "MESSAGE-INTEGRITY";
	case STUN_ATTR_ERR_CODE:          return "ERROR-CODE";
	case STUN_ATTR_UNKNOWN_ATTR:      return "UNKNOWN-ATTRIBUTE";
	case STUN_ATTR_CHANNEL_NUMBER:    return "CHANNEL-NUMBER";
	case STUN_ATTR_LIFETIME:          return "LIFETIME";
	case STUN_ATTR_XOR_PEER_ADDR:     return "XOR-PEER-ADDRESS";
	case STUN_ATTR_DATA:              return "DATA";
	case STUN_ATTR_REALM:             return "REALM";
	case STUN_ATTR_NONCE:             return "NONCE";
	case STUN_ATTR_XOR_RELAY_ADDR:    return "XOR-RELAYED-ADDRESS";
	case STUN_ATTR_REQ_ADDR_FAMILY:   return "REQUESTED-ADDRESS-FAMILY";
	case STUN_ATTR_EVEN_PORT:         return "EVEN_PORT";
	case STUN_ATTR_REQ_TRANSPORT:     return "REQUESTED-TRANSPORT";
	case STUN_ATTR_DONT_FRAGMENT:     return "DONT-FRAGMENT";
	case STUN_ATTR_XOR_MAPPED_ADDR:   return "XOR-MAPPED-ADDRESS";
	case STUN_ATTR_RSV_TOKEN:         return "RESERVATION-TOKEN";
	case STUN_ATTR_PRIORITY:          return "PRIORITY";
	case STUN_ATTR_USE_CAND:          return "USE-CANDIDATE";
	case STUN_ATTR_PADDING:           return "PADDING";
	case STUN_ATTR_RESP_PORT:         return "RESPONSE-PORT";
	case STUN_ATTR_SOFTWARE:          return "SOFTWARE";
	case STUN_ATTR_ALT_SERVER:        return "ALTERNATE-SERVER";
	case STUN_ATTR_FINGERPRINT:       return "FINGERPRINT";
	case STUN_ATTR_CONTROLLING:       return "ICE-CONTROLLING";
	case STUN_ATTR_CONTROLLED:        return "ICE-CONTROLLED";
	case STUN_ATTR_RESP_ORIGIN:       return "RESPONSE-ORIGIN";
	case STUN_ATTR_OTHER_ADDR:        return "OTHER-ADDR";
	default:                          return "???";
	}
}


void stun_attr_dump(const struct stun_attr *a)
{
	uint32_t i;
	size_t len;

	if (!a)
		return;

	(void)re_printf(" %-25s", stun_attr_name(a->type));

	switch (a->type) {

	case STUN_ATTR_MAPPED_ADDR:
	case STUN_ATTR_XOR_PEER_ADDR:
	case STUN_ATTR_XOR_RELAY_ADDR:
	case STUN_ATTR_XOR_MAPPED_ADDR:
	case STUN_ATTR_ALT_SERVER:
	case STUN_ATTR_RESP_ORIGIN:
	case STUN_ATTR_OTHER_ADDR:
		(void)re_printf("%J", &a->v.sa);
		break;

	case STUN_ATTR_CHANGE_REQ:
		(void)re_printf("ip=%u port=%u", a->v.change_req.ip,
				a->v.change_req.port);
		break;

	case STUN_ATTR_USERNAME:
	case STUN_ATTR_REALM:
	case STUN_ATTR_NONCE:
	case STUN_ATTR_SOFTWARE:
		(void)re_printf("%s", a->v.str);
		break;

	case STUN_ATTR_MSG_INTEGRITY:
		(void)re_printf("%w", a->v.msg_integrity,
				sizeof(a->v.msg_integrity));
		break;

	case STUN_ATTR_ERR_CODE:
		(void)re_printf("%u %s", a->v.err_code.code,
				a->v.err_code.reason);
		break;

	case STUN_ATTR_UNKNOWN_ATTR:
		for (i=0; i<a->v.unknown_attr.typec; i++)
			(void)re_printf("0x%04x ", a->v.unknown_attr.typev[i]);
		break;

	case STUN_ATTR_CHANNEL_NUMBER:
		(void)re_printf("0x%04x", a->v.uint16);
		break;

	case STUN_ATTR_LIFETIME:
	case STUN_ATTR_PRIORITY:
		(void)re_printf("%u", a->v.uint32);
		break;

	case STUN_ATTR_DATA:
	case STUN_ATTR_PADDING:
		len = min(mbuf_get_left(&a->v.mb), 16);
		(void)re_printf("%w%s (%zu bytes)", mbuf_buf(&a->v.mb), len,
				mbuf_get_left(&a->v.mb) > 16 ? "..." : "",
				mbuf_get_left(&a->v.mb));
		break;

	case STUN_ATTR_REQ_ADDR_FAMILY:
	case STUN_ATTR_REQ_TRANSPORT:
		(void)re_printf("%u", a->v.uint8);
		break;

	case STUN_ATTR_EVEN_PORT:
		(void)re_printf("r=%u", a->v.even_port.r);
		break;

	case STUN_ATTR_DONT_FRAGMENT:
	case STUN_ATTR_USE_CAND:
		/* no value */
		break;

	case STUN_ATTR_RSV_TOKEN:
		(void)re_printf("0x%016llx", a->v.rsv_token);
		break;

	case STUN_ATTR_RESP_PORT:
		(void)re_printf("%u", a->v.uint16);
		break;

	case STUN_ATTR_FINGERPRINT:
		(void)re_printf("0x%08x", a->v.fingerprint);
		break;

	case STUN_ATTR_CONTROLLING:
	case STUN_ATTR_CONTROLLED:
		(void)re_printf("%llu", a->v.uint64);
		break;

	default:
		(void)re_printf("???");
		break;
	}

	(void)re_printf("\n");
}
