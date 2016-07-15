/**
 * @file sdp/util.c  SDP utility functions
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sdp.h>


/**
 * Decode RTP Header Extension SDP attribute value
 *
 * @param ext Extension-map object
 * @param val SDP attribute value
 *
 * @return 0 for success, otherwise errorcode
 */
int sdp_extmap_decode(struct sdp_extmap *ext, const char *val)
{
	struct pl id, dir;

	if (!ext || !val)
		return EINVAL;

	if (re_regex(val, strlen(val), "[0-9]+[/]*[a-z]* [^ ]+[ ]*[^ ]*",
		     &id, NULL, &dir, &ext->name, NULL, &ext->attrs))
		return EBADMSG;

	ext->dir_set = false;
	ext->dir = SDP_SENDRECV;

	if (pl_isset(&dir)) {

		ext->dir_set = true;

		if      (!pl_strcmp(&dir, "sendonly")) ext->dir = SDP_SENDONLY;
		else if (!pl_strcmp(&dir, "sendrecv")) ext->dir = SDP_SENDRECV;
		else if (!pl_strcmp(&dir, "recvonly")) ext->dir = SDP_RECVONLY;
		else if (!pl_strcmp(&dir, "inactive")) ext->dir = SDP_INACTIVE;
		else ext->dir_set = false;
	}

	ext->id = pl_u32(&id);

	return 0;
}
