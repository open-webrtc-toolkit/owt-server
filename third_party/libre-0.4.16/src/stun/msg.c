/**
 * @file stun/msg.c  STUN message encoding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_md5.h>
#include <re_sha.h>
#include <re_hmac.h>
#include <re_crc32.h>
#include <re_stun.h>
#include "stun.h"


enum {
	MI_SIZE = 24,
	FP_SIZE = 8
};


/**
   Defines a STUN Message object

   <pre>

   .---------------------.      /|\           /|\
   | STUN Header         |       |             |
   |---------------------|       |             |
   |         ....        |       |--------.    |
   |      N Attributes   |       |        |    |----.
   |         ....        |      \|/       |    |    |
   |---------------------|                |    |    |
   |  MESSAGE-INTEGRITY  | <-(HMAC-SHA1)--'   \|/   |
   |---------------------|                          |
   |     FINGERPRINT     | <-(CRC-32)---------------'
   '---------------------'
   </pre>
*/
struct stun_msg {
	struct stun_hdr hdr;
	struct list attrl;
	struct mbuf *mb;
	size_t start;
};


static uint32_t fingerprint(const uint8_t *buf, size_t len)
{
	return (uint32_t)crc32(0, buf, (unsigned int)len) ^ 0x5354554e;
}


static void destructor(void *arg)
{
	struct stun_msg *msg = arg;

	list_flush(&msg->attrl);
	mem_deref(msg->mb);
}


/**
 * Decode a buffer to a STUN Message
 *
 * @param msgpp Pointer to allocation STUN message
 * @param mb    Buffer containing the raw STUN packet
 * @param ua    Unknown attributes (optional)
 *
 * @return 0 if success, otherwise errorcode
 *
 * @note `mb' will be referenced
 */
int stun_msg_decode(struct stun_msg **msgpp, struct mbuf *mb,
		    struct stun_unknown_attr *ua)
{
	struct stun_msg *msg;
	struct stun_hdr hdr;
	size_t start, extra;
	int err;

	if (!msgpp || !mb)
		return EINVAL;

	start = mb->pos;

	err = stun_hdr_decode(mb, &hdr);
	if (err) {
		mb->pos = start;
		return err;
	}

	msg = mem_zalloc(sizeof(*msg), destructor);
	if (!msg) {
		mb->pos = start;
		return ENOMEM;
	}

	msg->hdr = hdr;
	msg->mb = mem_ref(mb);
	msg->start = start;

	if (ua)
		ua->typec = 0;

	/* mbuf_get_left(mb) >= hdr.len checked in stun_hdr_decode() above */
	extra = mbuf_get_left(mb) - hdr.len;

	while (mbuf_get_left(mb) - extra >= 4) {

		struct stun_attr *attr;

		err = stun_attr_decode(&attr, mb, hdr.tid, ua);
		if (err)
			break;

		list_append(&msg->attrl, &attr->le, attr);
	}

	if (err)
		mem_deref(msg);
	else
		*msgpp = msg;

	mb->pos = start;

	return err;
}


/**
 * Get the STUN message type
 *
 * @param msg STUN Message
 *
 * @return STUN Message type
 */
uint16_t stun_msg_type(const struct stun_msg *msg)
{
	return msg ? msg->hdr.type : 0;
}


/**
 * Get the STUN message class
 *
 * @param msg STUN Message
 *
 * @return STUN Message class
 */
uint16_t stun_msg_class(const struct stun_msg *msg)
{
	return STUN_CLASS(stun_msg_type(msg));
}


/**
 * Get the STUN message method
 *
 * @param msg STUN Message
 *
 * @return STUN Message method
 */
uint16_t stun_msg_method(const struct stun_msg *msg)
{
	return STUN_METHOD(stun_msg_type(msg));
}


/**
 * Get the STUN message Transaction-ID
 *
 * @param msg STUN Message
 *
 * @return STUN Message Transaction-ID
 */
const uint8_t *stun_msg_tid(const struct stun_msg *msg)
{
	return msg ? msg->hdr.tid : NULL;
}


/**
 * Check if a STUN Message has the magic cookie
 *
 * @param msg STUN Message
 *
 * @return true if Magic Cookie, otherwise false
 */
bool stun_msg_mcookie(const struct stun_msg *msg)
{
	return msg && (STUN_MAGIC_COOKIE == msg->hdr.cookie);
}


/**
 * Lookup a STUN attribute in a STUN message
 *
 * @param msg  STUN Message
 * @param type STUN Attribute type
 *
 * @return STUN Attribute if found, otherwise NULL
 */
struct stun_attr *stun_msg_attr(const struct stun_msg *msg, uint16_t type)
{
	struct le *le = msg ? list_head(&msg->attrl) : NULL;

	while (le) {
		struct stun_attr *attr = le->data;

		le = le->next;

		if (attr->type == type)
			return attr;
	}

	return NULL;
}


/**
 * Apply a function handler to all STUN attribute
 *
 * @param msg  STUN Message
 * @param h    Attribute handler
 * @param arg  Handler argument
 *
 * @return STUN attribute if handler returned true, otherwise NULL
 */
struct stun_attr *stun_msg_attr_apply(const struct stun_msg *msg,
				      stun_attr_h *h, void *arg)
{
	struct le *le = msg ? list_head(&msg->attrl) : NULL;

	while (le) {
		struct stun_attr *attr = le->data;

		le = le->next;

		if (h && h(attr, arg))
			return (attr);
	}

	return NULL;
}


/**
 * Encode a STUN message
 *
 * @param mb      Buffer to encode message into
 * @param method  STUN Method
 * @param class   STUN Method class
 * @param tid     Transaction ID
 * @param ec      STUN error code (optional)
 * @param key     Authentication key (optional)
 * @param keylen  Number of bytes in authentication key
 * @param fp      Use STUN Fingerprint attribute
 * @param padding Padding byte
 * @param attrc   Number of attributes to encode (variable arguments)
 * @param ap      Variable list of attribute-tuples
 *                Each attribute has 2 arguments, attribute type and value
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_msg_vencode(struct mbuf *mb, uint16_t method, uint8_t class,
		     const uint8_t *tid, const struct stun_errcode *ec,
		     const uint8_t *key, size_t keylen, bool fp,
		     uint8_t padding, uint32_t attrc, va_list ap)
{
	struct stun_hdr hdr;
	size_t start;
	int err = 0;
	uint32_t i;

	if (!mb || !tid)
		return EINVAL;

	start = mb->pos;
	mb->pos += STUN_HEADER_SIZE;

	hdr.type   = STUN_TYPE(method, class);
	hdr.cookie = STUN_MAGIC_COOKIE;
	memcpy(hdr.tid, tid, STUN_TID_SIZE);

	if (ec)
		err |= stun_attr_encode(mb, STUN_ATTR_ERR_CODE, ec,
					NULL, padding);

	for (i=0; i<attrc; i++) {

		uint16_t type = va_arg(ap, int);
		const void *v = va_arg(ap, const void *);

		if (!v)
			continue;

		err |= stun_attr_encode(mb, type, v, hdr.tid, padding);
	}

	/* header */
	hdr.len = mb->pos - start - STUN_HEADER_SIZE + (key ? MI_SIZE : 0);
	mb->pos = start;
	err |= stun_hdr_encode(mb, &hdr);
	mb->pos += hdr.len - (key ? MI_SIZE : 0);

	if (key) {
		uint8_t mi[20];

		mb->pos = start;
		hmac_sha1(key, keylen, mbuf_buf(mb), mbuf_get_left(mb),
			  mi, sizeof(mi));

		mb->pos += STUN_HEADER_SIZE + hdr.len - MI_SIZE;
		err |= stun_attr_encode(mb, STUN_ATTR_MSG_INTEGRITY, mi,
					NULL, padding);
	}

	if (fp) {
		uint32_t fprnt;

		/* header */
		hdr.len = mb->pos - start - STUN_HEADER_SIZE + FP_SIZE;
		mb->pos = start;
		err |= stun_hdr_encode(mb, &hdr);

		mb->pos = start;
		fprnt = fingerprint(mbuf_buf(mb), mbuf_get_left(mb));

		mb->pos += STUN_HEADER_SIZE + hdr.len - FP_SIZE;
		err |= stun_attr_encode(mb, STUN_ATTR_FINGERPRINT, &fprnt,
					NULL, padding);
	}

	return err;
}


/**
 * Encode a STUN message
 *
 * @param mb      Buffer to encode message into
 * @param method  STUN Method
 * @param class   STUN Method class
 * @param tid     Transaction ID
 * @param ec      STUN error code (optional)
 * @param key     Authentication key (optional)
 * @param keylen  Number of bytes in authentication key
 * @param fp      Use STUN Fingerprint attribute
 * @param padding Padding byte
 * @param attrc   Number of attributes to encode (variable arguments)
 * @param ...     Variable list of attribute-tuples
 *                Each attribute has 2 arguments, attribute type and value
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_msg_encode(struct mbuf *mb, uint16_t method, uint8_t class,
		    const uint8_t *tid, const struct stun_errcode *ec,
		    const uint8_t *key, size_t keylen, bool fp,
		    uint8_t padding, uint32_t attrc, ...)
{
	va_list ap;
	int err;

	va_start(ap, attrc);
	err = stun_msg_vencode(mb, method, class, tid, ec, key, keylen, fp,
			       padding, attrc, ap);
	va_end(ap);

	return err;
}


/**
 * Verify the Message-Integrity of a STUN message
 *
 * @param msg    STUN Message
 * @param key    Authentication key
 * @param keylen Number of bytes in authentication key
 *
 * @return 0 if verified, otherwise errorcode
 */
int stun_msg_chk_mi(const struct stun_msg *msg, const uint8_t *key,
		    size_t keylen)
{
	uint8_t hmac[SHA_DIGEST_LENGTH];
	struct stun_attr *mi, *fp;

	if (!msg)
		return EINVAL;

	mi = stun_msg_attr(msg, STUN_ATTR_MSG_INTEGRITY);
	if (!mi)
		return EPROTO;

	msg->mb->pos = msg->start;

	fp = stun_msg_attr(msg, STUN_ATTR_FINGERPRINT);
	if (fp) {
		((struct stun_msg *)msg)->hdr.len -= FP_SIZE;
		(void)stun_hdr_encode(msg->mb, &msg->hdr);
		msg->mb->pos -= STUN_HEADER_SIZE;
	}

	hmac_sha1(key, keylen, mbuf_buf(msg->mb),
		  STUN_HEADER_SIZE + msg->hdr.len - MI_SIZE,
		  hmac, sizeof(hmac));

	if (fp) {
		((struct stun_msg *)msg)->hdr.len += FP_SIZE;
		(void)stun_hdr_encode(msg->mb, &msg->hdr);
		msg->mb->pos -= STUN_HEADER_SIZE;
	}

	if (memcmp(mi->v.msg_integrity, hmac, SHA_DIGEST_LENGTH))
		return EBADMSG;

	return 0;
}


/**
 * Check the Fingerprint of a STUN message
 *
 * @param msg STUN Message
 *
 * @return 0 if fingerprint matches, otherwise errorcode
 */
int stun_msg_chk_fingerprint(const struct stun_msg *msg)
{
	struct stun_attr *fp;
	uint32_t fprnt;

	if (!msg)
		return EINVAL;

	fp = stun_msg_attr(msg, STUN_ATTR_FINGERPRINT);
	if (!fp)
		return EPROTO;

	msg->mb->pos = msg->start;

	fprnt = fingerprint(mbuf_buf(msg->mb),
			    STUN_HEADER_SIZE + msg->hdr.len - FP_SIZE);

	if (fprnt != fp->v.fingerprint)
		return EBADMSG;

	return 0;
}


static bool attr_print(const struct stun_attr *attr, void *arg)
{
	(void)arg;

	stun_attr_dump(attr);

	return false;
}


/**
 * Print a STUN message to STDOUT
 *
 * @param msg STUN Message
 */
void stun_msg_dump(const struct stun_msg *msg)
{
	if (!msg)
		return;

	(void)re_printf("%s %s (len=%u cookie=%08x tid=%w)\n",
			stun_method_name(stun_msg_method(msg)),
			stun_class_name(stun_msg_class(msg)),
			msg->hdr.len, msg->hdr.cookie,
			msg->hdr.tid, sizeof(msg->hdr.tid));

	stun_msg_attr_apply(msg, attr_print, NULL);
}
