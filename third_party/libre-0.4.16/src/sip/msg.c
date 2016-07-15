/**
 * @file sip/msg.c  SIP Message decode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_sys.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_fmt.h>
#include <re_uri.h>
#include <re_udp.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


enum {
	HDR_HASH_SIZE = 32,
	STARTLINE_MAX = 8192,
};


static void hdr_destructor(void *arg)
{
	struct sip_hdr *hdr = arg;

	list_unlink(&hdr->le);
	hash_unlink(&hdr->he);
}


static void destructor(void *arg)
{
	struct sip_msg *msg = arg;

	list_flush(&msg->hdrl);
	hash_flush(msg->hdrht);
	mem_deref(msg->hdrht);
	mem_deref(msg->sock);
	mem_deref(msg->mb);
}


static enum sip_hdrid hdr_hash(const struct pl *name)
{
	if (!name->l)
		return SIP_HDR_NONE;

	if (name->l > 1) {
		switch (name->p[0]) {

		case 'x':
		case 'X':
			if (name->p[1] == '-')
				return SIP_HDR_NONE;

			/*@fallthrough@*/

		default:
			return (enum sip_hdrid)
				(hash_joaat_ci(name->p, name->l) & 0xfff);
		}
	}

	/* compact headers */
	switch (tolower(name->p[0])) {

	case 'a': return SIP_HDR_ACCEPT_CONTACT;
	case 'b': return SIP_HDR_REFERRED_BY;
	case 'c': return SIP_HDR_CONTENT_TYPE;
	case 'd': return SIP_HDR_REQUEST_DISPOSITION;
	case 'e': return SIP_HDR_CONTENT_ENCODING;
	case 'f': return SIP_HDR_FROM;
	case 'i': return SIP_HDR_CALL_ID;
	case 'j': return SIP_HDR_REJECT_CONTACT;
	case 'k': return SIP_HDR_SUPPORTED;
	case 'l': return SIP_HDR_CONTENT_LENGTH;
	case 'm': return SIP_HDR_CONTACT;
	case 'n': return SIP_HDR_IDENTITY_INFO;
	case 'o': return SIP_HDR_EVENT;
	case 'r': return SIP_HDR_REFER_TO;
	case 's': return SIP_HDR_SUBJECT;
	case 't': return SIP_HDR_TO;
	case 'u': return SIP_HDR_ALLOW_EVENTS;
	case 'v': return SIP_HDR_VIA;
	case 'x': return SIP_HDR_SESSION_EXPIRES;
	case 'y': return SIP_HDR_IDENTITY;
	default:  return SIP_HDR_NONE;
	}
}


static inline bool hdr_comma_separated(enum sip_hdrid id)
{
	switch (id) {

	case SIP_HDR_ACCEPT:
	case SIP_HDR_ACCEPT_CONTACT:
	case SIP_HDR_ACCEPT_ENCODING:
	case SIP_HDR_ACCEPT_LANGUAGE:
	case SIP_HDR_ACCEPT_RESOURCE_PRIORITY:
	case SIP_HDR_ALERT_INFO:
	case SIP_HDR_ALLOW:
	case SIP_HDR_ALLOW_EVENTS:
	case SIP_HDR_AUTHENTICATION_INFO:
	case SIP_HDR_CALL_INFO:
	case SIP_HDR_CONTACT:
	case SIP_HDR_CONTENT_ENCODING:
	case SIP_HDR_CONTENT_LANGUAGE:
	case SIP_HDR_ERROR_INFO:
	case SIP_HDR_HISTORY_INFO:
	case SIP_HDR_IN_REPLY_TO:
	case SIP_HDR_P_ASSERTED_IDENTITY:
	case SIP_HDR_P_ASSOCIATED_URI:
	case SIP_HDR_P_EARLY_MEDIA:
	case SIP_HDR_P_MEDIA_AUTHORIZATION:
	case SIP_HDR_P_PREFERRED_IDENTITY:
	case SIP_HDR_P_REFUSED_URI_LIST:
	case SIP_HDR_P_VISITED_NETWORK_ID:
	case SIP_HDR_PATH:
	case SIP_HDR_PERMISSION_MISSING:
	case SIP_HDR_PROXY_REQUIRE:
	case SIP_HDR_REASON:
	case SIP_HDR_RECORD_ROUTE:
	case SIP_HDR_REJECT_CONTACT:
	case SIP_HDR_REQUEST_DISPOSITION:
	case SIP_HDR_REQUIRE:
	case SIP_HDR_RESOURCE_PRIORITY:
	case SIP_HDR_ROUTE:
	case SIP_HDR_SECURITY_CLIENT:
	case SIP_HDR_SECURITY_SERVER:
	case SIP_HDR_SECURITY_VERIFY:
	case SIP_HDR_SERVICE_ROUTE:
	case SIP_HDR_SUPPORTED:
	case SIP_HDR_TRIGGER_CONSENT:
	case SIP_HDR_UNSUPPORTED:
	case SIP_HDR_VIA:
	case SIP_HDR_WARNING:
		return true;

	default:
		return false;
	}
}


static inline int hdr_add(struct sip_msg *msg, const struct pl *name,
			  enum sip_hdrid id, const char *p, ssize_t l,
			  bool atomic, bool line)
{
	struct sip_hdr *hdr;
	int err = 0;

	hdr = mem_zalloc(sizeof(*hdr), hdr_destructor);
	if (!hdr)
		return ENOMEM;

	hdr->name  = *name;
	hdr->val.p = p;
	hdr->val.l = MAX(l, 0);
	hdr->id    = id;

	switch (id) {

	case SIP_HDR_VIA:
	case SIP_HDR_ROUTE:
		if (!atomic)
			break;

		hash_append(msg->hdrht, id, &hdr->he, mem_ref(hdr));
		list_append(&msg->hdrl, &hdr->le, mem_ref(hdr));
		break;

	default:
		if (atomic)
			hash_append(msg->hdrht, id, &hdr->he, mem_ref(hdr));
		if (line)
			list_append(&msg->hdrl, &hdr->le, mem_ref(hdr));
		break;
	}

	/* parse common headers */
	switch (id) {

	case SIP_HDR_VIA:
		if (!atomic || pl_isset(&msg->via.sentby))
			break;

		err = sip_via_decode(&msg->via, &hdr->val);
		break;

	case SIP_HDR_TO:
		err = sip_addr_decode((struct sip_addr *)&msg->to, &hdr->val);
		if (err)
			break;

		(void)msg_param_decode(&msg->to.params, "tag", &msg->to.tag);
		msg->to.val = hdr->val;
		break;

	case SIP_HDR_FROM:
		err = sip_addr_decode((struct sip_addr *)&msg->from,
				      &hdr->val);
		if (err)
			break;

		(void)msg_param_decode(&msg->from.params, "tag",
				       &msg->from.tag);
		msg->from.val = hdr->val;
		break;

	case SIP_HDR_CALL_ID:
		msg->callid = hdr->val;
		break;

	case SIP_HDR_CSEQ:
		err = sip_cseq_decode(&msg->cseq, &hdr->val);
		break;

	case SIP_HDR_MAX_FORWARDS:
		msg->maxfwd = hdr->val;
		break;

	case SIP_HDR_CONTENT_TYPE:
		err = msg_ctype_decode(&msg->ctyp, &hdr->val);
		break;

	case SIP_HDR_CONTENT_LENGTH:
		msg->clen = hdr->val;
		break;

	case SIP_HDR_EXPIRES:
		msg->expires = hdr->val;
		break;

	default:
		/* re_printf("%r = %u\n", &hdr->name, id); */
		break;
	}

	mem_deref(hdr);

	return err;
}


/**
 * Decode a SIP message
 *
 * @param msgp Pointer to allocated SIP Message
 * @param mb   Buffer containing SIP Message
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_msg_decode(struct sip_msg **msgp, struct mbuf *mb)
{
	struct pl x, y, z, e, name;
	const char *p, *v, *cv;
	struct sip_msg *msg;
	bool comsep, quote;
	enum sip_hdrid id = SIP_HDR_NONE;
	uint32_t ws, lf;
	size_t l;
	int err;

	if (!msgp || !mb)
		return EINVAL;

	p = (const char *)mbuf_buf(mb);
	l = mbuf_get_left(mb);

	if (re_regex(p, l, "[^ \t\r\n]+ [^ \t\r\n]+ [^\r\n]*[\r]*[\n]1",
		     &x, &y, &z, NULL, &e) || x.p != (char *)mbuf_buf(mb))
		return (l > STARTLINE_MAX) ? EBADMSG : ENODATA;

	msg = mem_zalloc(sizeof(*msg), destructor);
	if (!msg)
		return ENOMEM;

	err = hash_alloc(&msg->hdrht, HDR_HASH_SIZE);
	if (err)
		goto out;

	msg->tag = rand_u64();
	msg->mb  = mem_ref(mb);
	msg->req = (0 == pl_strcmp(&z, "SIP/2.0"));

	if (msg->req) {

		msg->met = x;
		msg->ruri = y;
		msg->ver = z;

		if (uri_decode(&msg->uri, &y)) {
			err = EBADMSG;
			goto out;
		}
	}
	else {
		msg->ver    = x;
		msg->scode  = pl_u32(&y);
		msg->reason = z;

		if (!msg->scode) {
			err = EBADMSG;
			goto out;
		}
	}

	l -= e.p + e.l - p;
	p = e.p + e.l;

	name.p = v = cv = NULL;
	name.l = ws = lf = 0;
	comsep = false;
	quote = false;

	for (; l > 0; p++, l--) {

		switch (*p) {

		case ' ':
		case '\t':
			lf = 0; /* folding */
			++ws;
			break;

		case '\r':
			++ws;
			break;

		case '\n':
			++ws;

			if (!lf++)
				break;

			++p; --l; /* eoh */

			/*@fallthrough@*/

		default:
			if (lf || (*p == ',' && comsep && !quote)) {

				if (!name.l) {
					err = EBADMSG;
					goto out;
				}

				err = hdr_add(msg, &name, id, cv ? cv : p,
					      cv ? p - cv - ws : 0,
					      true, cv == v && lf);
				if (err)
					goto out;

				if (!lf) { /* comma separated */
					cv = NULL;
					break;
				}

				if (cv != v) {
					err = hdr_add(msg, &name, id,
						      v ? v : p,
						      v ? p - v - ws : 0,
						      false, true);
					if (err)
						goto out;
				}

				if (lf > 1) { /* eoh */
					err = 0;
					goto out;
				}

				comsep = false;
				name.p = NULL;
				cv = v = NULL;
				lf = 0;
			}

			if (!name.p) {
				name.p = p;
				name.l = 0;
				ws = 0;
			}

			if (!name.l) {
				if (*p != ':') {
					ws = 0;
					break;
				}

				name.l = MAX((int)(p - name.p - ws), 0);
				if (!name.l) {
					err = EBADMSG;
					goto out;
				}

				id = hdr_hash(&name);
				comsep = hdr_comma_separated(id);
				break;
			}

			if (!cv) {
				quote = false;
				cv = p;
			}

			if (!v) {
				v = p;
			}

			if (*p == '"')
				quote = !quote;

			ws = 0;
			break;
		}
	}

	err = ENODATA;

 out:
	if (err)
		mem_deref(msg);
	else {
		*msgp = msg;
		mb->pos = mb->end - l;
	}

	return err;
}


/**
 * Get a SIP Header from a SIP Message
 *
 * @param msg SIP Message
 * @param id  SIP Header ID
 *
 * @return SIP Header if found, NULL if not found
 */
const struct sip_hdr *sip_msg_hdr(const struct sip_msg *msg, enum sip_hdrid id)
{
	return sip_msg_hdr_apply(msg, true, id, NULL, NULL);
}


/**
 * Apply a function handler to certain SIP Headers
 *
 * @param msg SIP Message
 * @param fwd True to traverse forwards, false to traverse backwards
 * @param id  SIP Header ID
 * @param h   Function handler
 * @param arg Handler argument
 *
 * @return SIP Header if handler returns true, otherwise NULL
 */
const struct sip_hdr *sip_msg_hdr_apply(const struct sip_msg *msg,
					bool fwd, enum sip_hdrid id,
					sip_hdr_h *h, void *arg)
{
	struct list *lst;
	struct le *le;

	if (!msg)
		return NULL;

	lst = hash_list(msg->hdrht, id);

	le = fwd ? list_head(lst) : list_tail(lst);

	while (le) {
		const struct sip_hdr *hdr = le->data;

		le = fwd ? le->next : le->prev;

		if (hdr->id != id)
			continue;

		if (!h || h(hdr, msg, arg))
			return hdr;
	}

	return NULL;
}


/**
 * Get an unknown SIP Header from a SIP Message
 *
 * @param msg  SIP Message
 * @param name Header name
 *
 * @return SIP Header if found, NULL if not found
 */
const struct sip_hdr *sip_msg_xhdr(const struct sip_msg *msg, const char *name)
{
	return sip_msg_xhdr_apply(msg, true, name, NULL, NULL);
}


/**
 * Apply a function handler to certain unknown SIP Headers
 *
 * @param msg  SIP Message
 * @param fwd  True to traverse forwards, false to traverse backwards
 * @param name SIP Header name
 * @param h    Function handler
 * @param arg  Handler argument
 *
 * @return SIP Header if handler returns true, otherwise NULL
 */
const struct sip_hdr *sip_msg_xhdr_apply(const struct sip_msg *msg,
					 bool fwd, const char *name,
					 sip_hdr_h *h, void *arg)
{
	struct list *lst;
	struct le *le;
	struct pl pl;

	if (!msg || !name)
		return NULL;

	pl_set_str(&pl, name);

	lst = hash_list(msg->hdrht, hdr_hash(&pl));

	le = fwd ? list_head(lst) : list_tail(lst);

	while (le) {
		const struct sip_hdr *hdr = le->data;

		le = fwd ? le->next : le->prev;

		if (pl_casecmp(&hdr->name, &pl))
			continue;

		if (!h || h(hdr, msg, arg))
			return hdr;
	}

	return NULL;
}


static bool count_handler(const struct sip_hdr *hdr, const struct sip_msg *msg,
			  void *arg)
{
	uint32_t *n = arg;
	(void)hdr;
	(void)msg;

	++(*n);

	return false;
}


/**
 * Count the number of SIP Headers
 *
 * @param msg SIP Message
 * @param id  SIP Header ID
 *
 * @return Number of SIP Headers
 */
uint32_t sip_msg_hdr_count(const struct sip_msg *msg, enum sip_hdrid id)
{
	uint32_t n = 0;

	sip_msg_hdr_apply(msg, true, id, count_handler, &n);

	return n;
}


/**
 * Count the number of unknown SIP Headers
 *
 * @param msg  SIP Message
 * @param name SIP Header name
 *
 * @return Number of SIP Headers
 */
uint32_t sip_msg_xhdr_count(const struct sip_msg *msg, const char *name)
{
	uint32_t n = 0;

	sip_msg_xhdr_apply(msg, true, name, count_handler, &n);

	return n;
}


static bool value_handler(const struct sip_hdr *hdr, const struct sip_msg *msg,
			  void *arg)
{
	(void)msg;

	return 0 == pl_strcasecmp(&hdr->val, (const char *)arg);
}


/**
 * Check if a SIP Header matches a certain value
 *
 * @param msg   SIP Message
 * @param id    SIP Header ID
 * @param value Header value to check
 *
 * @return True if value matches, false if not
 */
bool sip_msg_hdr_has_value(const struct sip_msg *msg, enum sip_hdrid id,
			   const char *value)
{
	return NULL != sip_msg_hdr_apply(msg, true, id, value_handler,
					 (void *)value);
}


/**
 * Check if an unknown SIP Header matches a certain value
 *
 * @param msg   SIP Message
 * @param name  SIP Header name
 * @param value Header value to check
 *
 * @return True if value matches, false if not
 */
bool sip_msg_xhdr_has_value(const struct sip_msg *msg, const char *name,
			    const char *value)
{
	return NULL != sip_msg_xhdr_apply(msg, true, name, value_handler,
					  (void *)value);
}


/**
 * Print a SIP Message to stdout
 *
 * @param msg SIP Message
 */
void sip_msg_dump(const struct sip_msg *msg)
{
	struct le *le;
	uint32_t i;

	if (!msg)
		return;

	for (i=0; i<HDR_HASH_SIZE; i++) {

		le = list_head(hash_list(msg->hdrht, i));

		while (le) {
			const struct sip_hdr *hdr = le->data;

			le = le->next;

			(void)re_printf("%02u '%r'='%r'\n", i, &hdr->name,
					&hdr->val);
		}
	}

	le = list_head(&msg->hdrl);

	while (le) {
		const struct sip_hdr *hdr = le->data;

		le = le->next;

		(void)re_printf("%02u '%r'='%r'\n", hdr->id, &hdr->name,
				&hdr->val);
	}
}
