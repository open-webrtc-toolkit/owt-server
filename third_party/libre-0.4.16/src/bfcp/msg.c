/**
 * @file bfcp/msg.c BFCP Message
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
#include <re_tmr.h>
#include <re_bfcp.h>
#include "bfcp.h"


enum {
	BFCP_HDR_SIZE = 12,
};


static void destructor(void *arg)
{
	struct bfcp_msg *msg = arg;

	list_flush(&msg->attrl);
}


static int hdr_encode(struct mbuf *mb, uint8_t ver, bool r,
		      enum bfcp_prim prim, uint16_t len, uint32_t confid,
		      uint16_t tid, uint16_t userid)
{
	int err;

	err  = mbuf_write_u8(mb, (ver << 5) | ((r ? 1 : 0) << 4));
	err |= mbuf_write_u8(mb, prim);
	err |= mbuf_write_u16(mb, htons(len));
	err |= mbuf_write_u32(mb, htonl(confid));
	err |= mbuf_write_u16(mb, htons(tid));
	err |= mbuf_write_u16(mb, htons(userid));

	return err;
}


static int hdr_decode(struct bfcp_msg *msg, struct mbuf *mb)
{
	uint8_t b;

	if (mbuf_get_left(mb) < BFCP_HDR_SIZE)
		return ENODATA;

	b = mbuf_read_u8(mb);

	msg->ver    = b >> 5;
	msg->r      = (b >> 4) & 1;
	msg->f      = (b >> 3) & 1;
	msg->prim   = mbuf_read_u8(mb);
	msg->len    = ntohs(mbuf_read_u16(mb));
	msg->confid = ntohl(mbuf_read_u32(mb));
	msg->tid    = ntohs(mbuf_read_u16(mb));
	msg->userid = ntohs(mbuf_read_u16(mb));

	if (msg->ver != BFCP_VER1 && msg->ver != BFCP_VER2)
		return EBADMSG;

	/* fragmentation not supported */
	if (msg->f)
		return ENOSYS;

	if (mbuf_get_left(mb) < (size_t)(4*msg->len))
		return ENODATA;

	return 0;
}


/**
 * Encode a BFCP message with variable arguments
 *
 * @param mb      Mbuf to encode into
 * @param ver     Protocol version
 * @param r       Transaction responder flag
 * @param prim    BFCP Primitive
 * @param confid  Conference ID
 * @param tid     Transaction ID
 * @param userid  User ID
 * @param attrc   Number of attributes
 * @param ap      Variable argument of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_msg_vencode(struct mbuf *mb, uint8_t ver, bool r, enum bfcp_prim prim,
		     uint32_t confid, uint16_t tid, uint16_t userid,
		     unsigned attrc, va_list *ap)
{
	size_t start, len;
	int err;

	if (!mb)
		return EINVAL;

	start = mb->pos;
	mb->pos += BFCP_HDR_SIZE;

	err = bfcp_attrs_vencode(mb, attrc, ap);
	if (err)
		return err;

	/* header */
	len = mb->pos - start - BFCP_HDR_SIZE;
	mb->pos = start;
	err = hdr_encode(mb, ver, r, prim, (uint16_t)(len/4), confid, tid,
			 userid);
	mb->pos += len;

	return err;
}


/**
 * Encode a BFCP message
 *
 * @param mb      Mbuf to encode into
 * @param ver     Protocol version
 * @param r       Transaction responder flag
 * @param prim    BFCP Primitive
 * @param confid  Conference ID
 * @param tid     Transaction ID
 * @param userid  User ID
 * @param attrc   Number of attributes
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_msg_encode(struct mbuf *mb, uint8_t ver, bool r, enum bfcp_prim prim,
		    uint32_t confid, uint16_t tid, uint16_t userid,
		    unsigned attrc, ...)
{
	va_list ap;
	int err;

	va_start(ap, attrc);
	err = bfcp_msg_vencode(mb, ver, r, prim, confid, tid, userid,
			       attrc, &ap);
	va_end(ap);

	return err;
}


/**
 * Decode a BFCP message from a buffer
 *
 * @param msgp Pointer to allocated and decoded BFCP message
 * @param mb   Mbuf to decode from
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_msg_decode(struct bfcp_msg **msgp, struct mbuf *mb)
{
	struct bfcp_msg *msg;
	size_t start;
	int err;

	if (!msgp || !mb)
		return EINVAL;

	msg = mem_zalloc(sizeof(*msg), destructor);
	if (!msg)
		return ENOMEM;

	start = mb->pos;

	err = hdr_decode(msg, mb);
	if (err) {
		mb->pos = start;
		goto out;
	}

	err = bfcp_attrs_decode(&msg->attrl, mb, 4*msg->len, &msg->uma);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(msg);
	else
		*msgp = msg;

	return err;
}


/**
 * Get a BFCP attribute from a BFCP message
 *
 * @param msg  BFCP message
 * @param type Attribute type
 *
 * @return Matching BFCP attribute if found, otherwise NULL
 */
struct bfcp_attr *bfcp_msg_attr(const struct bfcp_msg *msg,
				enum bfcp_attrib type)
{
	if (!msg)
		return NULL;

	return bfcp_attrs_find(&msg->attrl, type);
}


/**
 * Apply a function handler to all attributes in a BFCP message
 *
 * @param msg  BFCP message
 * @param h    Handler
 * @param arg  Handler argument
 *
 * @return BFCP attribute returned by handler, or NULL
 */
struct bfcp_attr *bfcp_msg_attr_apply(const struct bfcp_msg *msg,
				      bfcp_attr_h *h, void *arg)
{
	if (!msg)
		return NULL;

	return bfcp_attrs_apply(&msg->attrl, h, arg);
}


/**
 * Print a BFCP message
 *
 * @param pf  Print function
 * @param msg BFCP message
 *
 * @return 0 if success, otherwise errorcode
 */
int bfcp_msg_print(struct re_printf *pf, const struct bfcp_msg *msg)
{
	int err;

	if (!msg)
		return 0;

	err = re_hprintf(pf, "%s (confid=%u tid=%u userid=%u)\n",
			 bfcp_prim_name(msg->prim), msg->confid,
			 msg->tid, msg->userid);

	err |= bfcp_attrs_print(pf, &msg->attrl, 0);

	return err;
}


/**
 * Get the BFCP primitive name
 *
 * @param prim BFCP primitive
 *
 * @return String with BFCP primitive name
 */
const char *bfcp_prim_name(enum bfcp_prim prim)
{
	switch (prim) {

	case BFCP_FLOOR_REQUEST:        return "FloorRequest";
	case BFCP_FLOOR_RELEASE:        return "FloorRelease";
	case BFCP_FLOOR_REQUEST_QUERY:  return "FloorRequestQuery";
	case BFCP_FLOOR_REQUEST_STATUS: return "FloorRequestStatus";
	case BFCP_USER_QUERY:           return "UserQuery";
	case BFCP_USER_STATUS:          return "UserStatus";
	case BFCP_FLOOR_QUERY:          return "FloorQuery";
	case BFCP_FLOOR_STATUS:         return "FloorStatus";
	case BFCP_CHAIR_ACTION:         return "ChairAction";
	case BFCP_CHAIR_ACTION_ACK:     return "ChairActionAck";
	case BFCP_HELLO:                return "Hello";
	case BFCP_HELLO_ACK:            return "HelloAck";
	case BFCP_ERROR:                return "Error";
	case BFCP_FLOOR_REQ_STATUS_ACK: return "FloorRequestStatusAck";
	case BFCP_FLOOR_STATUS_ACK:     return "FloorStatusAck";
	case BFCP_GOODBYE:              return "Goodbye";
	case BFCP_GOODBYE_ACK:          return "GoodbyeAck";
	default:                        return "???";
	}
}
