/**
 * @file sdp/media.c  SDP Media
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdlib.h>
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sdp.h>
#include "sdp.h"


static void destructor(void *arg)
{
	struct sdp_media *m = arg;
	unsigned i;

	list_flush(&m->lfmtl);
	list_flush(&m->rfmtl);
	list_flush(&m->rattrl);
	list_flush(&m->lattrl);

	if (m->le.list) {
		m->disabled = true;
		m->ench     = NULL;
		mem_ref(m);
		return;
	}

	for (i=0; i<ARRAY_SIZE(m->protov); i++)
		mem_deref(m->protov[i]);

	list_unlink(&m->le);
	mem_deref(m->name);
	mem_deref(m->proto);
	mem_deref(m->uproto);
}


static int media_alloc(struct sdp_media **mp, struct list *list)
{
	struct sdp_media *m;
	int i;

	m = mem_zalloc(sizeof(*m), destructor);
	if (!m)
		return ENOMEM;

	list_append(list, &m->le, m);

	m->ldir  = SDP_SENDRECV;
	m->rdir  = SDP_SENDRECV;
	m->dynpt = RTP_DYNPT_START;

	sa_init(&m->laddr, AF_INET);
	sa_init(&m->raddr, AF_INET);
	sa_init(&m->laddr_rtcp, AF_INET);
	sa_init(&m->raddr_rtcp, AF_INET);

	for (i=0; i<SDP_BANDWIDTH_MAX; i++) {
		m->lbwv[i] = -1;
		m->rbwv[i] = -1;
	}

	*mp = m;

	return 0;
}


/**
 * Add a media line to an SDP Session
 *
 * @param mp    Pointer to allocated SDP Media line object
 * @param sess  SDP Session
 * @param name  Media name
 * @param port  Port number
 * @param proto Transport protocol
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_media_add(struct sdp_media **mp, struct sdp_session *sess,
		  const char *name, uint16_t port, const char *proto)
{
	struct sdp_media *m;
	int err;

	if (!sess || !name || !proto)
		return EINVAL;

	err = media_alloc(&m, &sess->lmedial);
	if (err)
		return err;

	err  = str_dup(&m->name, name);
	err |= str_dup(&m->proto, proto);
	if (err)
		goto out;

	sa_set_port(&m->laddr, port);

 out:
	if (err)
		mem_deref(m);
	else if (mp)
		*mp = m;

	return err;
}


/**
 * Add a remote SDP media line to an SDP Session
 *
 * @param mp    Pointer to allocated SDP Media line object
 * @param sess  SDP Session
 * @param name  Media name
 * @param proto Transport protocol
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_media_radd(struct sdp_media **mp, struct sdp_session *sess,
		   const struct pl *name, const struct pl *proto)
{
	struct sdp_media *m;
	int err;

	if (!mp || !sess || !name || !proto)
		return EINVAL;

	err = media_alloc(&m, &sess->medial);
	if (err)
		return err;

	m->disabled = true;

	err  = pl_strdup(&m->name, name);
	err |= pl_strdup(&m->proto, proto);

	if (err)
		mem_deref(m);
	else
		*mp = m;

	return err;
}


/**
 * Reset the remote part of an SDP Media line
 *
 * @param m SDP Media line
 */
void sdp_media_rreset(struct sdp_media *m)
{
	int i;

	if (!m)
		return;

	sa_init(&m->raddr, AF_INET);
	sa_init(&m->raddr_rtcp, AF_INET);

	list_flush(&m->rfmtl);
	list_flush(&m->rattrl);

	m->rdir = SDP_SENDRECV;

	for (i=0; i<SDP_BANDWIDTH_MAX; i++)
		m->rbwv[i] = -1;
}


/**
 * Compare media line protocols
 *
 * @param m      SDP Media line
 * @param proto  Transport protocol
 * @param update Update media protocol if match is found in alternate set
 *
 * @return True if matching, False if not
 */
bool sdp_media_proto_cmp(struct sdp_media *m, const struct pl *proto,
			 bool update)
{
	unsigned i;

	if (!m || !proto)
		return false;

	if (!pl_strcmp(proto, m->proto))
		return true;

	for (i=0; i<ARRAY_SIZE(m->protov); i++) {

		if (!m->protov[i] || pl_strcmp(proto, m->protov[i]))
			continue;

		if (update) {
			mem_deref(m->proto);
			m->proto = mem_ref(m->protov[i]);
		}

		return true;
	}

	return false;
}


/**
 * Find an SDP Media line from name and transport protocol
 *
 * @param sess  SDP Session
 * @param name  Media name
 * @param proto Transport protocol
 * @param update_proto Update media transport protocol
 *
 * @return Matching media line if found, NULL if not found
 */
struct sdp_media *sdp_media_find(const struct sdp_session *sess,
				 const struct pl *name,
				 const struct pl *proto,
				 bool update_proto)
{
	struct le *le;

	if (!sess || !name || !proto)
		return NULL;

	for (le=sess->lmedial.head; le; le=le->next) {

		struct sdp_media *m = le->data;

		if (pl_strcmp(name, m->name))
			continue;

		if (!sdp_media_proto_cmp(m, proto, update_proto))
			continue;

		return m;
	}

	return NULL;
}


/**
 * Align the locate/remote formats of an SDP Media line
 *
 * @param m     SDP Media line
 * @param offer True if SDP Offer, False if SDP Answer
 */
void sdp_media_align_formats(struct sdp_media *m, bool offer)
{
	struct sdp_format *rfmt, *lfmt;
	struct le *rle, *lle;

	if (!m || m->disabled || !sa_port(&m->raddr) || m->fmt_ignore)
		return;

	for (lle=m->lfmtl.head; lle; lle=lle->next) {

		lfmt = lle->data;

		lfmt->rparams = mem_deref(lfmt->rparams);
		lfmt->sup = false;
	}

	for (rle=m->rfmtl.head; rle; rle=rle->next) {

		rfmt = rle->data;

		for (lle=m->lfmtl.head; lle; lle=lle->next) {

			lfmt = lle->data;

			if (sdp_format_cmp(lfmt, rfmt))
				break;
		}

		if (!lle) {
			rfmt->sup = false;
			continue;
		}

		mem_deref(lfmt->rparams);
		lfmt->rparams = mem_ref(rfmt->params);

		lfmt->sup = true;
		rfmt->sup = true;

		if (rfmt->ref)
			rfmt->data = mem_deref(rfmt->data);
		else
			rfmt->data = NULL;

		if (lfmt->ref)
			rfmt->data = mem_ref(lfmt->data);
		else
			rfmt->data = lfmt->data;

		rfmt->ref = lfmt->ref;

		if (offer) {
			mem_deref(lfmt->id);
			lfmt->id = mem_ref(rfmt->id);
			lfmt->pt = atoi(lfmt->id ? lfmt->id : "");

			list_unlink(&lfmt->le);
			list_append(&m->lfmtl, &lfmt->le, lfmt);
		}
	}

	if (offer) {

		for (lle=m->lfmtl.tail; lle; ) {

			lfmt = lle->data;

			lle = lle->prev;

			if (!lfmt->sup) {
				list_unlink(&lfmt->le);
				list_append(&m->lfmtl, &lfmt->le, lfmt);
			}
		}
	}
}


/**
 * Set alternative protocols for an SDP Media line
 *
 * @param m      SDP Media line
 * @param protoc Number of alternative protocols
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_media_set_alt_protos(struct sdp_media *m, unsigned protoc, ...)
{
	const char *proto;
	int err = 0;
	unsigned i;
	va_list ap;

	if (!m)
		return EINVAL;

	va_start(ap, protoc);

	for (i=0; i<ARRAY_SIZE(m->protov); i++) {

		m->protov[i] = mem_deref(m->protov[i]);

		if (i >= protoc)
			continue;

		proto = va_arg(ap, const char *);
		if (proto)
			err |= str_dup(&m->protov[i], proto);
	}

	va_end(ap);

	return err;
}


/**
 * Set SDP Media line encode handler
 *
 * @param m    SDP Media line
 * @param ench Encode handler
 * @param arg  Encode handler argument
 */
void sdp_media_set_encode_handler(struct sdp_media *m, sdp_media_enc_h *ench,
				  void *arg)
{
	if (!m)
		return;

	m->ench = ench;
	m->arg  = arg;
}


/**
 * Set an SDP Media line to ignore formats
 *
 * @param m          SDP Media line
 * @param fmt_ignore True for ignore formats, otherwise false
 */
void sdp_media_set_fmt_ignore(struct sdp_media *m, bool fmt_ignore)
{
	if (!m)
		return;

	m->fmt_ignore = fmt_ignore;
}


/**
 * Set an SDP Media line to enabled/disabled
 *
 * @param m        SDP Media line
 * @param disabled True for disabled, False for enabled
 */
void sdp_media_set_disabled(struct sdp_media *m, bool disabled)
{
	if (!m)
		return;

	m->disabled = disabled;
}


/**
 * Set the local port number of an SDP Media line
 *
 * @param m    SDP Media line
 * @param port Port number
 */
void sdp_media_set_lport(struct sdp_media *m, uint16_t port)
{
	if (!m)
		return;

	sa_set_port(&m->laddr, port);
}


/**
 * Set the local network address of an SDP media line
 *
 * @param m     SDP Media line
 * @param laddr Local network address
 */
void sdp_media_set_laddr(struct sdp_media *m, const struct sa *laddr)
{
	if (!m || !laddr)
		return;

	m->laddr = *laddr;
}


/**
 * Set a local bandwidth of an SDP Media line
 *
 * @param m    SDP Media line
 * @param type Bandwidth type
 * @param bw   Bandwidth value
 */
void sdp_media_set_lbandwidth(struct sdp_media *m, enum sdp_bandwidth type,
			      int32_t bw)
{
	if (!m || type >= SDP_BANDWIDTH_MAX)
		return;

	m->lbwv[type] = bw;
}


/**
 * Set the local RTCP port number of an SDP Media line
 *
 * @param m    SDP Media line
 * @param port RTCP Port number
 */
void sdp_media_set_lport_rtcp(struct sdp_media *m, uint16_t port)
{
	if (!m)
		return;

	sa_set_port(&m->laddr_rtcp, port);
}


/**
 * Set the local RTCP network address of an SDP media line
 *
 * @param m     SDP Media line
 * @param laddr Local RTCP network address
 */
void sdp_media_set_laddr_rtcp(struct sdp_media *m, const struct sa *laddr)
{
	if (!m || !laddr)
		return;

	m->laddr_rtcp = *laddr;
}


/**
 * Set the local direction flag of an SDP Media line
 *
 * @param m   SDP Media line
 * @param dir Media direction flag
 */
void sdp_media_set_ldir(struct sdp_media *m, enum sdp_dir dir)
{
	if (!m)
		return;

	m->ldir = dir;
}


/**
 * Set a local attribute of an SDP Media line
 *
 * @param m       SDP Media line
 * @param replace True to replace attribute, False to append
 * @param name    Attribute name
 * @param value   Formatted attribute value
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_media_set_lattr(struct sdp_media *m, bool replace,
			const char *name, const char *value, ...)
{
	va_list ap;
	int err;

	if (!m || !name)
		return EINVAL;

	if (replace)
		sdp_attr_del(&m->lattrl, name);

	va_start(ap, value);
	err = sdp_attr_addv(&m->lattrl, name, value, ap);
	va_end(ap);

	return err;
}


/**
 * Delete a local attribute of an SDP Media line
 *
 * @param m    SDP Media line
 * @param name Attribute name
 */
void sdp_media_del_lattr(struct sdp_media *m, const char *name)
{
	if (!m || !name)
		return;

	sdp_attr_del(&m->lattrl, name);
}


const char *sdp_media_proto(const struct sdp_media *m)
{
	return m ? m->proto : NULL;
}


/**
 * Get the remote port number of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return Remote port number
 */
uint16_t sdp_media_rport(const struct sdp_media *m)
{
	return m ? sa_port(&m->raddr) : 0;
}


/**
 * Get the remote network address of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return Remote network address
 */
const struct sa *sdp_media_raddr(const struct sdp_media *m)
{
	return m ? &m->raddr : NULL;
}


/**
 * Get the local network address of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return Local network address
 */
const struct sa *sdp_media_laddr(const struct sdp_media *m)
{
	return m ? &m->laddr : NULL;
}


/**
 * Get the remote RTCP network address of an SDP Media line
 *
 * @param m     SDP Media line
 * @param raddr On return, contains remote RTCP network address
 */
void sdp_media_raddr_rtcp(const struct sdp_media *m, struct sa *raddr)
{
	if (!m || !raddr)
		return;

	if (sa_isset(&m->raddr_rtcp, SA_ALL)) {
		*raddr = m->raddr_rtcp;
	}
	else if (sa_isset(&m->raddr_rtcp, SA_PORT)) {
		*raddr = m->raddr;
		sa_set_port(raddr, sa_port(&m->raddr_rtcp));
	}
	else {
		uint16_t port = sa_port(&m->raddr);

		*raddr = m->raddr;
		sa_set_port(raddr, port ? port + 1 : 0);
	}
}


/**
 * Get a remote bandwidth of an SDP Media line
 *
 * @param m    SDP Media line
 * @param type Bandwidth type
 *
 * @return Remote bandwidth value
 */
int32_t sdp_media_rbandwidth(const struct sdp_media *m,
			      enum sdp_bandwidth type)
{
	if (!m || type >= SDP_BANDWIDTH_MAX)
		return 0;

	return m->rbwv[type];
}


/**
 * Get the local media direction of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return Local media direction
 */
enum sdp_dir sdp_media_ldir(const struct sdp_media *m)
{
	return m ? m->ldir : SDP_INACTIVE;
}


/**
 * Get the remote media direction of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return Remote media direction
 */
enum sdp_dir sdp_media_rdir(const struct sdp_media *m)
{
	return m ? m->rdir : SDP_INACTIVE;
}


/**
 * Get the combined media direction of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return Combined media direction
 */
enum sdp_dir sdp_media_dir(const struct sdp_media *m)
{
	return m ? (enum sdp_dir)(m->ldir & m->rdir) : SDP_INACTIVE;
}


/**
 * Find a local SDP format from a payload type
 *
 * @param m  SDP Media line
 * @param pt Payload type
 *
 * @return Local SDP format if found, NULL if not found
 */
const struct sdp_format *sdp_media_lformat(const struct sdp_media *m, int pt)
{
	struct le *le;

	if (!m)
		return NULL;

	for (le=m->lfmtl.head; le; le=le->next) {

		const struct sdp_format *fmt = le->data;

		if (pt == fmt->pt)
			return fmt;
	}

	return NULL;
}


/**
 * Find a remote SDP format from a format name
 *
 * @param m    SDP Media line
 * @param name Format name
 *
 * @return Remote SDP format if found, NULL if not found
 */
const struct sdp_format *sdp_media_rformat(const struct sdp_media *m,
					   const char *name)
{
	struct le *le;

	if (!m || !sa_port(&m->raddr))
		return NULL;

	for (le=m->rfmtl.head; le; le=le->next) {

		const struct sdp_format *fmt = le->data;

		if (!fmt->sup)
			continue;

		if (name && str_casecmp(name, fmt->name))
			continue;

		return fmt;
	}

	return NULL;
}


/**
 * Find an SDP Format of an SDP Media line
 *
 * @param m     SDP Media line
 * @param local True if local media, False if remote
 * @param id    SDP format id
 * @param pt    Payload type
 * @param name  Format name
 * @param srate Sampling rate
 * @param ch    Number of channels
 *
 * @return SDP Format if found, NULL if not found
 */
struct sdp_format *sdp_media_format(const struct sdp_media *m,
				    bool local, const char *id,
				    int pt, const char *name,
				    int32_t srate, int8_t ch)
{
	return sdp_media_format_apply(m, local, id, pt, name, srate, ch,
				      NULL, NULL);
}


/**
 * Apply a function handler to all matching SDP formats
 *
 * @param m     SDP Media line
 * @param local True if local media, False if remote
 * @param id    SDP format id
 * @param pt    Payload type
 * @param name  Format name
 * @param srate Sampling rate
 * @param ch    Number of channels
 * @param fmth  SDP Format handler
 * @param arg   Handler argument
 *
 * @return SDP Format if found, NULL if not found
 */
struct sdp_format *sdp_media_format_apply(const struct sdp_media *m,
					  bool local, const char *id,
					  int pt, const char *name,
					  int32_t srate, int8_t ch,
					  sdp_format_h *fmth, void *arg)
{
	struct le *le;

	if (!m)
		return NULL;

	le = local ? m->lfmtl.head : m->rfmtl.head;

	while (le) {

		struct sdp_format *fmt = le->data;

		le = le->next;

		if (id && (!fmt->id || strcmp(id, fmt->id)))
			continue;

		if (pt >= 0 && pt != fmt->pt)
			continue;

		if (name && str_casecmp(name, fmt->name))
			continue;

		if (srate >= 0 && (uint32_t)srate != fmt->srate)
			continue;

		if (ch >= 0 && (uint8_t)ch != fmt->ch)
			continue;

		if (!fmth || fmth(fmt, arg))
			return fmt;
	}

	return NULL;
}


/**
 * Get the list of SDP Formats
 *
 * @param m     SDP Media line
 * @param local True if local, False if remote
 *
 * @return List of SDP Formats
 */
const struct list *sdp_media_format_lst(const struct sdp_media *m, bool local)
{
	if (!m)
		return NULL;

	return local ? &m->lfmtl : &m->rfmtl;
}


/**
 * Get a remote attribute from an SDP Media line
 *
 * @param m     SDP Media line
 * @param name  Attribute name
 *
 * @return Attribute value, NULL if not found
 */
const char *sdp_media_rattr(const struct sdp_media *m, const char *name)
{
	if (!m || !name)
		return NULL;

	return sdp_attr_apply(&m->rattrl, name, NULL, NULL);
}


/**
 * Get a remote attribute from an SDP Media line or the SDP session
 *
 * @param m     SDP Media line
 * @param sess  SDP Session
 * @param name  Attribute name
 *
 * @return Attribute value, NULL if not found
 */
const char *sdp_media_session_rattr(const struct sdp_media *m,
				    const struct sdp_session *sess,
				    const char *name)
{
	const char *val;

	val = sdp_media_rattr(m, name);
	if (!val)
		val = sdp_session_rattr(sess, name);

	return val;
}


/**
 * Apply a function handler to all matching remote attributes
 *
 * @param m     SDP Media line
 * @param name  Attribute name
 * @param attrh Attribute handler
 * @param arg   Handler argument
 *
 * @return Attribute value if match
 */
const char *sdp_media_rattr_apply(const struct sdp_media *m, const char *name,
				  sdp_attr_h *attrh, void *arg)
{
	if (!m)
		return NULL;

	return sdp_attr_apply(&m->rattrl, name, attrh, arg);
}


/**
 * Get the name of an SDP Media line
 *
 * @param m SDP Media line
 *
 * @return SDP Media line name
 */
const char *sdp_media_name(const struct sdp_media *m)
{
	return m ? m->name : NULL;
}


/**
 * Print SDP Media line debug information
 *
 * @param pf Print function for output
 * @param m  SDP Media line
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_media_debug(struct re_printf *pf, const struct sdp_media *m)
{
	struct le *le;
	int err;

	if (!m)
		return 0;

	err  = re_hprintf(pf, "%s %s\n", m->name, m->proto);

	err |= re_hprintf(pf, "  local formats:\n");

	for (le=m->lfmtl.head; le; le=le->next)
		err |= re_hprintf(pf, "    %H\n", sdp_format_debug, le->data);

	err |= re_hprintf(pf, "  remote formats:\n");

	for (le=m->rfmtl.head; le; le=le->next)
		err |= re_hprintf(pf, "    %H\n", sdp_format_debug, le->data);

	err |= re_hprintf(pf, "  local attributes:\n");

	for (le=m->lattrl.head; le; le=le->next)
		err |= re_hprintf(pf, "    %H\n", sdp_attr_debug, le->data);

	err |= re_hprintf(pf, "  remote attributes:\n");

	for (le=m->rattrl.head; le; le=le->next)
		err |= re_hprintf(pf, "    %H\n", sdp_attr_debug, le->data);

	return err;
}
