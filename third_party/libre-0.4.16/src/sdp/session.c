/**
 * @file sdp/session.c  SDP Session
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sa.h>
#include <re_sdp.h>
#include <re_sys.h>
#include "sdp.h"


static void destructor(void *arg)
{
	struct sdp_session *sess = arg;

	list_flush(&sess->lmedial);
	list_flush(&sess->medial);
	list_flush(&sess->rattrl);
	list_flush(&sess->lattrl);
}


/**
 * Allocate a new SDP Session
 *
 * @param sessp Pointer to allocated SDP Session object
 * @param laddr Local network address
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_session_alloc(struct sdp_session **sessp, const struct sa *laddr)
{
	struct sdp_session *sess;
	int err = 0, i;

	if (!sessp || !laddr)
		return EINVAL;

	sess = mem_zalloc(sizeof(*sess), destructor);
	if (!sess)
		return ENOMEM;

	sess->laddr = *laddr;
	sess->id    = rand_u32();
	sess->ver   = rand_u32() & 0x7fffffff;
	sess->rdir  = SDP_SENDRECV;

	sa_init(&sess->raddr, AF_INET);

	for (i=0; i<SDP_BANDWIDTH_MAX; i++) {
		sess->lbwv[i] = -1;
		sess->rbwv[i] = -1;
	}

	if (err)
		mem_deref(sess);
	else
		*sessp = sess;

	return err;
}


/**
 * Reset the remote side of an SDP Session
 *
 * @param sess SDP Session
 */
void sdp_session_rreset(struct sdp_session *sess)
{
	int i;

	if (!sess)
		return;

	sa_init(&sess->raddr, AF_INET);

	list_flush(&sess->rattrl);

	sess->rdir = SDP_SENDRECV;

	for (i=0; i<SDP_BANDWIDTH_MAX; i++)
		sess->rbwv[i] = -1;
}


/**
 * Set the local network address of an SDP Session
 *
 * @param sess  SDP Session
 * @param laddr Local network address
 */
void sdp_session_set_laddr(struct sdp_session *sess, const struct sa *laddr)
{
	if (!sess || !laddr)
		return;

	sess->laddr = *laddr;
}


/**
 * Set the local bandwidth of an SDP Session
 *
 * @param sess SDP Session
 * @param type Bandwidth type
 * @param bw   Bandwidth value
 */
void sdp_session_set_lbandwidth(struct sdp_session *sess,
				enum sdp_bandwidth type, int32_t bw)
{
	if (!sess || type >= SDP_BANDWIDTH_MAX)
		return;

	sess->lbwv[type] = bw;
}


/**
 * Set a local attribute of an SDP Session
 *
 * @param sess    SDP Session
 * @param replace True to replace any existing attributes, false to append
 * @param name    Attribute name
 * @param value   Formatted attribute value
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_session_set_lattr(struct sdp_session *sess, bool replace,
			  const char *name, const char *value, ...)
{
	va_list ap;
	int err;

	if (!sess || !name)
		return EINVAL;

	if (replace)
		sdp_attr_del(&sess->lattrl, name);

	va_start(ap, value);
	err = sdp_attr_addv(&sess->lattrl, name, value, ap);
	va_end(ap);

	return err;
}


/**
 * Delete a local attribute of an SDP Session
 *
 * @param sess SDP Session
 * @param name Attribute name
 */
void sdp_session_del_lattr(struct sdp_session *sess, const char *name)
{
	if (!sess || !name)
		return;

	sdp_attr_del(&sess->lattrl, name);
}


/**
 * Get the local bandwidth of an SDP Session
 *
 * @param sess SDP Session
 * @param type Bandwidth type
 *
 * @return Bandwidth value
 */
int32_t sdp_session_lbandwidth(const struct sdp_session *sess,
			       enum sdp_bandwidth type)
{
	if (!sess || type >= SDP_BANDWIDTH_MAX)
		return 0;

	return sess->lbwv[type];
}


/**
 * Get the remote bandwidth of an SDP Session
 *
 * @param sess SDP Session
 * @param type Bandwidth type
 *
 * @return Bandwidth value
 */
int32_t sdp_session_rbandwidth(const struct sdp_session *sess,
				enum sdp_bandwidth type)
{
	if (!sess || type >= SDP_BANDWIDTH_MAX)
		return 0;

	return sess->rbwv[type];
}


/**
 * Get a remote attribute of an SDP Session
 *
 * @param sess SDP Session
 * @param name Attribute name
 *
 * @return Attribute value if exist, NULL if not exist
 */
const char *sdp_session_rattr(const struct sdp_session *sess, const char *name)
{
	if (!sess || !name)
		return NULL;

	return sdp_attr_apply(&sess->rattrl, name, NULL, NULL);
}


/**
 * Apply a function handler of all matching remote attributes
 *
 * @param sess  SDP Session
 * @param name  Attribute name
 * @param attrh Attribute handler
 * @param arg   Handler argument
 *
 * @return Attribute value if match
 */
const char *sdp_session_rattr_apply(const struct sdp_session *sess,
				    const char *name,
				    sdp_attr_h *attrh, void *arg)
{
	if (!sess)
		return NULL;

	return sdp_attr_apply(&sess->rattrl, name, attrh, arg);
}


/**
 * Get the list of media-lines from an SDP Session
 *
 * @param sess  SDP Session
 * @param local True for local, False for remote
 *
 * @return List of media-lines
 */
const struct list *sdp_session_medial(const struct sdp_session *sess,
				      bool local)
{
	if (!sess)
		return NULL;

	return local ? &sess->lmedial : &sess->medial;
}


/**
 * Print SDP Session debug information
 *
 * @param pf   Print function for output
 * @param sess SDP Session
 *
 * @return 0 if success, otherwise errorcode
 */
int sdp_session_debug(struct re_printf *pf, const struct sdp_session *sess)
{
	struct le *le;
	int err;

	if (!sess)
		return 0;

	err = re_hprintf(pf, "SDP session\n");

	err |= re_hprintf(pf, "  local attributes:\n");

	for (le=sess->lattrl.head; le; le=le->next)
		err |= re_hprintf(pf, "    %H\n", sdp_attr_debug, le->data);

	err |= re_hprintf(pf, "  remote attributes:\n");

	for (le=sess->rattrl.head; le; le=le->next)
		err |= re_hprintf(pf, "    %H\n", sdp_attr_debug, le->data);

	err |= re_hprintf(pf, "session media:\n");

	for (le=sess->medial.head; le; le=le->next)
		err |= sdp_media_debug(pf, le->data);

	err |= re_hprintf(pf, "local media:\n");

	for (le=sess->lmedial.head; le; le=le->next)
		err |= sdp_media_debug(pf, le->data);

	return err;
}
