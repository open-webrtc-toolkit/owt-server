/**
 * @file stun/hdr.c  STUN Header encoding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_stun.h>
#include "stun.h"


int stun_hdr_encode(struct mbuf *mb, const struct stun_hdr *hdr)
{
	int err = 0;

	if (!mb || !hdr)
		return EINVAL;

	err |= mbuf_write_u16(mb, htons(hdr->type & 0x3fff));
	err |= mbuf_write_u16(mb, htons(hdr->len));
	err |= mbuf_write_u32(mb, htonl(hdr->cookie));
	err |= mbuf_write_mem(mb, hdr->tid, sizeof(hdr->tid));

	return err;
}


int stun_hdr_decode(struct mbuf *mb, struct stun_hdr *hdr)
{
	if (!mb || !hdr)
		return EINVAL;

	if (mbuf_get_left(mb) < STUN_HEADER_SIZE)
		return EBADMSG;

	hdr->type = ntohs(mbuf_read_u16(mb));
	if (hdr->type & 0xc000)
		return EBADMSG;

	hdr->len = ntohs(mbuf_read_u16(mb));
	if (hdr->len & 0x3)
		return EBADMSG;

	hdr->cookie = ntohl(mbuf_read_u32(mb));
	(void)mbuf_read_mem(mb, hdr->tid, sizeof(hdr->tid));

	if (mbuf_get_left(mb) < hdr->len)
		return EBADMSG;

	return 0;
}


const char *stun_class_name(uint16_t class)
{
	switch (class) {

	case STUN_CLASS_REQUEST:      return "Request";
	case STUN_CLASS_INDICATION:   return "Indication";
	case STUN_CLASS_SUCCESS_RESP: return "Success Response";
	case STUN_CLASS_ERROR_RESP:   return "Error Response";
	default:                      return "???";
	}
}


const char *stun_method_name(uint16_t method)
{
	switch (method) {

	case STUN_METHOD_BINDING:    return "Binding";
	case STUN_METHOD_ALLOCATE:   return "Allocate";
	case STUN_METHOD_REFRESH:    return "Refresh";
	case STUN_METHOD_SEND:       return "Send";
	case STUN_METHOD_DATA:       return "Data";
	case STUN_METHOD_CREATEPERM: return "CreatePermission";
	case STUN_METHOD_CHANBIND:   return "ChannelBind";
	default:                     return "???";
	}
}
