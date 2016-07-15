/**
 * @file ctype.c  Content-Type decode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_msg.h>


/**
 * Decode a pointer-length string into Content-Type header
 *
 * @param ctype Content-Type header
 * @param pl    Pointer-length string
 *
 * @return 0 for success, otherwise errorcode
 */
int msg_ctype_decode(struct msg_ctype *ctype, const struct pl *pl)
{
	struct pl ws;

	if (!ctype || !pl)
		return EINVAL;

	if (re_regex(pl->p, pl->l,
		     "[ \t\r\n]*[^ \t\r\n;/]+[ \t\r\n]*/[ \t\r\n]*[^ \t\r\n;]+"
		     "[^]*",
		     &ws, &ctype->type, NULL, NULL, &ctype->subtype,
		     &ctype->params))
		return EBADMSG;

	if (ws.p != pl->p)
		return EBADMSG;

	return 0;
}


/**
 * Compare Content-Type
 *
 * @param ctype   Content-Type header
 * @param type    Media type
 * @param subtype Media sub-type
 *
 * @return true if match, false if no match
 */
bool msg_ctype_cmp(const struct msg_ctype *ctype,
		   const char *type, const char *subtype)
{
	if (!ctype || !type || !subtype)
		return false;

	if (pl_strcasecmp(&ctype->type, type))
		return false;

	if (pl_strcasecmp(&ctype->subtype, subtype))
		return false;

	return true;
}
