/**
 * @file sdp/msg.c  SDP Message processing
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sdp.h>
#include "sdp.h"


static int attr_decode_fmtp(struct sdp_media *m, const struct pl *pl)
{
	struct sdp_format *fmt;
	struct pl id, params;

	if (!m)
		return 0;

	if (re_regex(pl->p, pl->l, "[^ ]+ [^]*", &id, &params))
		return EBADMSG;

	fmt = sdp_format_find(&m->rfmtl, &id);
	if (!fmt)
		return 0;

	fmt->params = mem_deref(fmt->params);

	return pl_strdup(&fmt->params, &params);
}


static int attr_decode_rtcp(struct sdp_media *m, const struct pl *pl)
{
	struct pl port, addr;
	int err = 0;

	if (!m)
		return 0;

	if (!re_regex(pl->p, pl->l, "[0-9]+ IN IP[46]1 [^ ]+",
		      &port, NULL, &addr)) {
		(void)sa_set(&m->raddr_rtcp, &addr, pl_u32(&port));
	}
	else if (!re_regex(pl->p, pl->l, "[0-9]+", &port)) {
		sa_set_port(&m->raddr_rtcp, pl_u32(&port));
	}
	else
		err = EBADMSG;

	return err;
}


static int attr_decode_rtpmap(struct sdp_media *m, const struct pl *pl)
{
	struct pl id, name, srate, ch;
	struct sdp_format *fmt;
	int err;

	if (!m)
		return 0;

	if (re_regex(pl->p, pl->l, "[^ ]+ [^/]+/[0-9]+[/]*[^]*",
		     &id, &name, &srate, NULL, &ch))
		return EBADMSG;

	fmt = sdp_format_find(&m->rfmtl, &id);
	if (!fmt)
		return 0;

	fmt->name = mem_deref(fmt->name);

	err = pl_strdup(&fmt->name, &name);
	if (err)
		return err;

	fmt->srate = pl_u32(&srate);
	fmt->ch = ch.l ? pl_u32(&ch) : 1;

	return 0;
}


static int attr_decode(struct sdp_session *sess, struct sdp_media *m,
		       enum sdp_dir *dir, const struct pl *pl)
{
	struct pl name, val;
	int err = 0;

	if (re_regex(pl->p, pl->l, "[^:]+:[^]+", &name, &val)) {
		name = *pl;
		val  = pl_null;
	}

	if (!pl_strcmp(&name, "fmtp"))
		err = attr_decode_fmtp(m, &val);

	else if (!pl_strcmp(&name, "inactive"))
		*dir = SDP_INACTIVE;

	else if (!pl_strcmp(&name, "recvonly"))
		*dir = SDP_SENDONLY;

	else if (!pl_strcmp(&name, "rtcp"))
		err = attr_decode_rtcp(m, &val);

	else if (!pl_strcmp(&name, "rtpmap"))
		err = attr_decode_rtpmap(m, &val);

	else if (!pl_strcmp(&name, "sendonly"))
		*dir = SDP_RECVONLY;

	else if (!pl_strcmp(&name, "sendrecv"))
		*dir = SDP_SENDRECV;

	else
		err = sdp_attr_add(m ? &m->rattrl : &sess->rattrl,
				   &name, &val);

	return err;
}


static int bandwidth_decode(int32_t *bwv, const struct pl *pl)
{
	struct pl type, bw;

	if (re_regex(pl->p, pl->l, "[^:]+:[0-9]+", &type, &bw))
		return EBADMSG;

	if (!pl_strcmp(&type, "CT"))
		bwv[SDP_BANDWIDTH_CT] = pl_u32(&bw);

	else if (!pl_strcmp(&type, "AS"))
		bwv[SDP_BANDWIDTH_AS] = pl_u32(&bw);

	else if (!pl_strcmp(&type, "RS"))
		bwv[SDP_BANDWIDTH_RS] = pl_u32(&bw);

	else if (!pl_strcmp(&type, "RR"))
		bwv[SDP_BANDWIDTH_RR] = pl_u32(&bw);

	else if (!pl_strcmp(&type, "TIAS"))
		bwv[SDP_BANDWIDTH_TIAS] = pl_u32(&bw);

	return 0;
}


static int conn_decode(struct sa *sa, const struct pl *pl)
{
	struct pl v;

	if (re_regex(pl->p, pl->l, "IN IP[46]1 [^ ]+", NULL, &v))
		return EBADMSG;

	(void)sa_set(sa, &v, sa_port(sa));

	return 0;
}


static int media_decode(struct sdp_media **mp, struct sdp_session *sess,
			bool offer, const struct pl *pl)
{
	struct pl name, port, proto, fmtv, fmt;
	struct sdp_media *m;
	int err;

	if (re_regex(pl->p, pl->l, "[a-z]+ [^ ]+ [^ ]+[^]*",
		     &name, &port, &proto, &fmtv))
		return EBADMSG;

	m = list_ledata(*mp ? (*mp)->le.next : sess->medial.head);
	if (!m) {
		if (!offer)
			return EPROTO;

		m = sdp_media_find(sess, &name, &proto, true);
		if (!m) {
			err = sdp_media_radd(&m, sess, &name, &proto);
			if (err)
				return err;
		}
		else {
			list_unlink(&m->le);
			list_append(&sess->medial, &m->le, m);
		}

		m->uproto = mem_deref(m->uproto);
	}
	else {
		if (pl_strcmp(&name, m->name))
			return offer ? ENOTSUP : EPROTO;

		m->uproto = mem_deref(m->uproto);

		if (!sdp_media_proto_cmp(m, &proto, offer)) {

			err = pl_strdup(&m->uproto, &proto);
			if (err)
				return err;
		}
	}

	while (!re_regex(fmtv.p, fmtv.l, " [^ ]+", &fmt)) {

		pl_advance(&fmtv, fmt.p + fmt.l - fmtv.p);

		err = sdp_format_radd(m, &fmt);
		if (err)
			return err;
	}

	m->raddr = sess->raddr;
	sa_set_port(&m->raddr, m->uproto ? 0 : pl_u32(&port));

	m->rdir = sess->rdir;

	*mp = m;

	return 0;
}


static int version_decode(const struct pl *pl)
{
	return pl_strcmp(pl, "0") ? ENOSYS : 0;
}


/**
 * Decode an SDP message into an SDP Session
 *
 * @param sess  SDP Session
 * @param mb    Memory buffer containing SDP message
 * @param offer True if SDP offer, False if SDP answer
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_decode(struct sdp_session *sess, struct mbuf *mb, bool offer)
{
	struct sdp_media *m;
	struct pl pl, val;
	struct le *le;
	char type = 0;
	int err = 0;

	if (!sess || !mb)
		return EINVAL;

	sdp_session_rreset(sess);

	for (le=sess->medial.head; le; le=le->next) {

		m = le->data;

		sdp_media_rreset(m);
	}

	pl.p = (const char *)mbuf_buf(mb);
	pl.l = mbuf_get_left(mb);

	m = NULL;

	for (;pl.l && !err; pl.p++, pl.l--) {

		switch (*pl.p) {

		case '\r':
		case '\n':
			if (!type)
				break;

			switch (type) {

			case 'a':
				err = attr_decode(sess, m,
						  m ? &m->rdir : &sess->rdir,
						  &val);
				break;

			case 'b':
				err = bandwidth_decode(m? m->rbwv : sess->rbwv,
						       &val);
				break;

			case 'c':
				err = conn_decode(m ? &m->raddr : &sess->raddr,
						  &val);
				break;

			case 'm':
				err = media_decode(&m, sess, offer, &val);
				break;

			case 'v':
				err = version_decode(&val);
				break;
			}

#if 0
			if (err)
				re_printf("** %c='%r': %m\n", type, &val, err);
#endif

			type = 0;
			break;

		default:
			if (type) {
				val.l++;
				break;
			}

			if (pl.l < 2 || *(pl.p + 1) != '=') {
				err = EBADMSG;
				break;
			}

			type  = *pl.p;
			val.p = pl.p + 2;
			val.l = 0;

			pl.p += 1;
			pl.l -= 1;
			break;
		}
	}

	if (err)
		return err;

	if (type)
		return EBADMSG;

	for (le=sess->medial.head; le; le=le->next)
		sdp_media_align_formats(le->data, offer);

	return 0;
}


static int media_encode(const struct sdp_media *m, struct mbuf *mb, bool offer)
{
	enum sdp_bandwidth i;
	const char *proto;
	int err, supc = 0;
	bool disabled;
	struct le *le;
	uint16_t port;

	for (le=m->lfmtl.head; le; le=le->next) {

		const struct sdp_format *fmt = le->data;

		if (fmt->sup)
			++supc;
	}

	if (m->uproto && !offer) {
		disabled = true;
		port = 0;
		proto = m->uproto;
	}
	else if (m->disabled || supc == 0 || (!offer && !sa_port(&m->raddr))) {
		disabled = true;
		port = 0;
		proto = m->proto;
	}
	else {
		disabled = false;
		port = sa_port(&m->laddr);
		proto = m->proto;
	}

	err = mbuf_printf(mb, "m=%s %u %s", m->name, port, proto);

	if (disabled) {
		err |= mbuf_write_str(mb, " 0\r\n");
		return err;
	}

	for (le=m->lfmtl.head; le; le=le->next) {

		const struct sdp_format *fmt = le->data;

		if (!fmt->sup)
			continue;

		err |= mbuf_printf(mb, " %s", fmt->id);
	}

	err |= mbuf_write_str(mb, "\r\n");

	if (sa_isset(&m->laddr, SA_ADDR)) {
		const int ipver = sa_af(&m->laddr) == AF_INET ? 4 : 6;
		err |= mbuf_printf(mb, "c=IN IP%d %j\r\n", ipver, &m->laddr);
	}

	for (i=SDP_BANDWIDTH_MIN; i<SDP_BANDWIDTH_MAX; i++) {

		if (m->lbwv[i] < 0)
			continue;

		err |= mbuf_printf(mb, "b=%s:%i\r\n",
				   sdp_bandwidth_name(i), m->lbwv[i]);
	}

	for (le=m->lfmtl.head; le; le=le->next) {

		const struct sdp_format *fmt = le->data;

		if (!fmt->sup || !str_isset(fmt->name))
			continue;

		err |= mbuf_printf(mb, "a=rtpmap:%s %s/%u",
				   fmt->id, fmt->name, fmt->srate);

		if (fmt->ch > 1)
			err |= mbuf_printf(mb, "/%u", fmt->ch);

		err |= mbuf_printf(mb, "\r\n");

		if (str_isset(fmt->params))
			err |= mbuf_printf(mb, "a=fmtp:%s %s\r\n",
					   fmt->id, fmt->params);
		if (fmt->ench)
			err |= fmt->ench(mb, fmt, offer, fmt->data);
	}

	if (sa_isset(&m->laddr_rtcp, SA_ALL))
		err |= mbuf_printf(mb, "a=rtcp:%u IN IP%d %j\r\n",
				   sa_port(&m->laddr_rtcp),
				   (AF_INET == sa_af(&m->laddr_rtcp)) ? 4 : 6,
				   &m->laddr_rtcp);
	else if (sa_isset(&m->laddr_rtcp, SA_PORT))
		err |= mbuf_printf(mb, "a=rtcp:%u\r\n",
				   sa_port(&m->laddr_rtcp));

	err |= mbuf_printf(mb, "a=%s\r\n",
			   sdp_dir_name(offer ? m->ldir : m->ldir & m->rdir));

	for (le = m->lattrl.head; le; le = le->next)
		err |= mbuf_printf(mb, "%H", sdp_attr_print, le->data);

	if (m->ench)
		err |= m->ench(mb, offer, m->arg);

	return err;
}


/**
 * Encode an SDP Session into a memory buffer
 *
 * @param mbp   Pointer to allocated memory buffer
 * @param sess  SDP Session
 * @param offer True if SDP Offer, False if SDP Answer
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_encode(struct mbuf **mbp, struct sdp_session *sess, bool offer)
{
	const int ipver = sa_af(&sess->laddr) == AF_INET ? 4 : 6;
	enum sdp_bandwidth i;
	struct mbuf *mb;
	struct le *le;
	int err;

	if (!mbp || !sess)
		return EINVAL;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err  = mbuf_printf(mb, "v=%u\r\n", SDP_VERSION);
	err |= mbuf_printf(mb, "o=- %u %u IN IP%d %j\r\n",
			   sess->id, sess->ver++, ipver, &sess->laddr);
	err |= mbuf_write_str(mb, "s=-\r\n");
	err |= mbuf_printf(mb, "c=IN IP%d %j\r\n", ipver, &sess->laddr);

	for (i=SDP_BANDWIDTH_MIN; i<SDP_BANDWIDTH_MAX; i++) {

		if (sess->lbwv[i] < 0)
			continue;

		err |= mbuf_printf(mb, "b=%s:%i\r\n",
				   sdp_bandwidth_name(i), sess->lbwv[i]);
	}

	err |= mbuf_write_str(mb, "t=0 0\r\n");

	for (le = sess->lattrl.head; le; le = le->next)
		err |= mbuf_printf(mb, "%H", sdp_attr_print, le->data);

	for (le=sess->lmedial.head; offer && le;) {

		struct sdp_media *m = le->data;

		le = le->next;

		if (m->disabled)
			continue;

		list_unlink(&m->le);
		list_append(&sess->medial, &m->le, m);
	}

	for (le=sess->medial.head; le; le=le->next) {

		struct sdp_media *m = le->data;

		err |= media_encode(m, mb, offer);
	}

	mb->pos = 0;

	if (err)
		mem_deref(mb);
	else
		*mbp = mb;

	return err;
}
