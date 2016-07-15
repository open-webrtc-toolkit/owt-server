/**
 * @file http/msg.c  HTTP Message decode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_fmt.h>
#include <re_msg.h>
#include <re_http.h>


enum {
	STARTLINE_MAX = 8192,
};


static void hdr_destructor(void *arg)
{
	struct http_hdr *hdr = arg;

	list_unlink(&hdr->le);
}


static void destructor(void *arg)
{
	struct http_msg *msg = arg;

	list_flush(&msg->hdrl);
	mem_deref(msg->mb);
}


static enum http_hdrid hdr_hash(const struct pl *name)
{
	if (!name->l)
		return HTTP_HDR_NONE;

	switch (name->p[0]) {

	case 'x':
	case 'X':
		if (name->l > 1 && name->p[1] == '-')
			return HTTP_HDR_NONE;

		break;
	}

	return (enum http_hdrid)(hash_joaat_ci(name->p, name->l) & 0xfff);
}


static inline bool hdr_comma_separated(enum http_hdrid id)
{
	switch (id) {

	case HTTP_HDR_ACCEPT:
	case HTTP_HDR_ACCEPT_CHARSET:
	case HTTP_HDR_ACCEPT_ENCODING:
	case HTTP_HDR_ACCEPT_LANGUAGE:
	case HTTP_HDR_ACCEPT_RANGES:
	case HTTP_HDR_ALLOW:
	case HTTP_HDR_CACHE_CONTROL:
	case HTTP_HDR_CONNECTION:
	case HTTP_HDR_CONTENT_ENCODING:
	case HTTP_HDR_CONTENT_LANGUAGE:
	case HTTP_HDR_EXPECT:
	case HTTP_HDR_IF_MATCH:
	case HTTP_HDR_IF_NONE_MATCH:
	case HTTP_HDR_PRAGMA:
	case HTTP_HDR_SEC_WEBSOCKET_EXTENSIONS:
	case HTTP_HDR_SEC_WEBSOCKET_PROTOCOL:
	case HTTP_HDR_SEC_WEBSOCKET_VERSION:
	case HTTP_HDR_TE:
	case HTTP_HDR_TRAILER:
	case HTTP_HDR_TRANSFER_ENCODING:
	case HTTP_HDR_UPGRADE:
	case HTTP_HDR_VARY:
	case HTTP_HDR_VIA:
	case HTTP_HDR_WARNING:
		return true;

	default:
		return false;
	}
}


static inline int hdr_add(struct http_msg *msg, const struct pl *name,
			  enum http_hdrid id, const char *p, ssize_t l)
{
	struct http_hdr *hdr;
	int err = 0;

	hdr = mem_zalloc(sizeof(*hdr), hdr_destructor);
	if (!hdr)
		return ENOMEM;

	hdr->name  = *name;
	hdr->val.p = p;
	hdr->val.l = MAX(l, 0);
	hdr->id    = id;

	list_append(&msg->hdrl, &hdr->le, hdr);

	/* parse common headers */
	switch (id) {

	case HTTP_HDR_CONTENT_TYPE:
		err = msg_ctype_decode(&msg->ctyp, &hdr->val);
		break;

	case HTTP_HDR_CONTENT_LENGTH:
		msg->clen = pl_u32(&hdr->val);
		break;

	default:
		break;
	}

	if (err)
		mem_deref(hdr);

	return err;
}


/**
 * Decode a HTTP message
 *
 * @param msgp Pointer to allocated HTTP Message
 * @param mb   Buffer containing HTTP Message
 * @param req  True for request, false for response
 *
 * @return 0 if success, otherwise errorcode
 */
int http_msg_decode(struct http_msg **msgp, struct mbuf *mb, bool req)
{
	struct pl b, s, e, name, scode;
	const char *p, *cv;
	struct http_msg *msg;
	bool comsep, quote;
	enum http_hdrid id = HTTP_HDR_NONE;
	uint32_t ws, lf;
	size_t l;
	int err;

	if (!msgp || !mb)
		return EINVAL;

	p = (const char *)mbuf_buf(mb);
	l = mbuf_get_left(mb);

	if (re_regex(p, l, "[\r\n]*[^\r\n]+[\r]*[\n]1", &b, &s, NULL, &e))
		return (l > STARTLINE_MAX) ? EBADMSG : ENODATA;

	msg = mem_zalloc(sizeof(*msg), destructor);
	if (!msg)
		return ENOMEM;

	msg->mb = mem_ref(mb);

	if (req) {
		if (re_regex(s.p, s.l, "[a-z]+ [^? ]+[^ ]* HTTP/[0-9.]+",
			     &msg->met, &msg->path, &msg->prm, &msg->ver) ||
		    msg->met.p != s.p) {
			err = EBADMSG;
			goto out;
		}
	}
	else {
		if (re_regex(s.p, s.l, "HTTP/[0-9.]+ [0-9]+[ ]*[^]*",
			     &msg->ver, &scode, NULL, &msg->reason) ||
		    msg->ver.p != s.p + 5) {
			err = EBADMSG;
			goto out;
		}

		msg->scode = pl_u32(&scode);
	}

	l -= e.p + e.l - p;
	p = e.p + e.l;

	name.p = cv = NULL;
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

			if (!name.p) {
				++p; --l; /* no headers */
				err = 0;
				goto out;
			}

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
					      cv ? p - cv - ws : 0);
				if (err)
					goto out;

				if (!lf) { /* comma separated */
					cv = NULL;
					break;
				}

				if (lf > 1) { /* eoh */
					err = 0;
					goto out;
				}

				comsep = false;
				name.p = NULL;
				cv = NULL;
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
 * Get a HTTP Header from a HTTP Message
 *
 * @param msg HTTP Message
 * @param id  HTTP Header ID
 *
 * @return HTTP Header if found, NULL if not found
 */
const struct http_hdr *http_msg_hdr(const struct http_msg *msg,
				    enum http_hdrid id)
{
	return http_msg_hdr_apply(msg, true, id, NULL, NULL);
}


/**
 * Apply a function handler to certain HTTP Headers
 *
 * @param msg HTTP Message
 * @param fwd True to traverse forwards, false to traverse backwards
 * @param id  HTTP Header ID
 * @param h   Function handler
 * @param arg Handler argument
 *
 * @return HTTP Header if handler returns true, otherwise NULL
 */
const struct http_hdr *http_msg_hdr_apply(const struct http_msg *msg,
					  bool fwd, enum http_hdrid id,
					  http_hdr_h *h, void *arg)
{
	struct le *le;

	if (!msg)
		return NULL;

	le = fwd ? msg->hdrl.head : msg->hdrl.tail;

	while (le) {
		const struct http_hdr *hdr = le->data;

		le = fwd ? le->next : le->prev;

		if (hdr->id != id)
			continue;

		if (!h || h(hdr, arg))
			return hdr;
	}

	return NULL;
}


/**
 * Get an unknown HTTP Header from a HTTP Message
 *
 * @param msg  HTTP Message
 * @param name Header name
 *
 * @return HTTP Header if found, NULL if not found
 */
const struct http_hdr *http_msg_xhdr(const struct http_msg *msg,
				     const char *name)
{
	return http_msg_xhdr_apply(msg, true, name, NULL, NULL);
}


/**
 * Apply a function handler to certain unknown HTTP Headers
 *
 * @param msg  HTTP Message
 * @param fwd  True to traverse forwards, false to traverse backwards
 * @param name HTTP Header name
 * @param h    Function handler
 * @param arg  Handler argument
 *
 * @return HTTP Header if handler returns true, otherwise NULL
 */
const struct http_hdr *http_msg_xhdr_apply(const struct http_msg *msg,
					   bool fwd, const char *name,
					   http_hdr_h *h, void *arg)
{
	struct le *le;
	struct pl pl;

	if (!msg || !name)
		return NULL;

	pl_set_str(&pl, name);

	le = fwd ? msg->hdrl.head : msg->hdrl.tail;

	while (le) {
		const struct http_hdr *hdr = le->data;

		le = fwd ? le->next : le->prev;

		if (pl_casecmp(&hdr->name, &pl))
			continue;

		if (!h || h(hdr, arg))
			return hdr;
	}

	return NULL;
}


static bool count_handler(const struct http_hdr *hdr, void *arg)
{
	uint32_t *n = arg;
	(void)hdr;

	++(*n);

	return false;
}


/**
 * Count the number of HTTP Headers
 *
 * @param msg HTTP Message
 * @param id  HTTP Header ID
 *
 * @return Number of HTTP Headers
 */
uint32_t http_msg_hdr_count(const struct http_msg *msg, enum http_hdrid id)
{
	uint32_t n = 0;

	http_msg_hdr_apply(msg, true, id, count_handler, &n);

	return n;
}


/**
 * Count the number of unknown HTTP Headers
 *
 * @param msg  HTTP Message
 * @param name HTTP Header name
 *
 * @return Number of HTTP Headers
 */
uint32_t http_msg_xhdr_count(const struct http_msg *msg, const char *name)
{
	uint32_t n = 0;

	http_msg_xhdr_apply(msg, true, name, count_handler, &n);

	return n;
}


static bool value_handler(const struct http_hdr *hdr, void *arg)
{
	return 0 == pl_strcasecmp(&hdr->val, (const char *)arg);
}


/**
 * Check if a HTTP Header matches a certain value
 *
 * @param msg   HTTP Message
 * @param id    HTTP Header ID
 * @param value Header value to check
 *
 * @return True if value matches, false if not
 */
bool http_msg_hdr_has_value(const struct http_msg *msg, enum http_hdrid id,
			    const char *value)
{
	return NULL != http_msg_hdr_apply(msg, true, id, value_handler,
					 (void *)value);
}


/**
 * Check if an unknown HTTP Header matches a certain value
 *
 * @param msg   HTTP Message
 * @param name  HTTP Header name
 * @param value Header value to check
 *
 * @return True if value matches, false if not
 */
bool http_msg_xhdr_has_value(const struct http_msg *msg, const char *name,
			     const char *value)
{
	return NULL != http_msg_xhdr_apply(msg, true, name, value_handler,
					  (void *)value);
}


/**
 * Print a HTTP Message
 *
 * @param pf  Print function for output
 * @param msg HTTP Message
 *
 * @return 0 if success, otherwise errorcode
 */
int http_msg_print(struct re_printf *pf, const struct http_msg *msg)
{
	struct le *le;
	int err;

	if (!msg)
		return 0;

	if (pl_isset(&msg->met))
		err = re_hprintf(pf, "%r %r%r HTTP/%r\n", &msg->met,
				 &msg->path, &msg->prm, &msg->ver);
	else
		err = re_hprintf(pf, "HTTP/%r %u %r\n", &msg->ver, msg->scode,
				 &msg->reason);

	for (le=msg->hdrl.head; le; le=le->next) {

		const struct http_hdr *hdr = le->data;

		err |= re_hprintf(pf, "%r: %r (%i)\n", &hdr->name, &hdr->val,
				  hdr->id);
	}

	return err;
}
