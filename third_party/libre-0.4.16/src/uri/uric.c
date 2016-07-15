/**
 * @file uric.c  URI component escaping/unescaping
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_uri.h>


#define DEBUG_MODULE "uric"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/** Defines the URI escape handler */
typedef bool (esc_h)(char c);


static bool is_mark(int c)
{
	switch (c) {

	case '-':
	case '_':
	case '.':
	case '!':
	case '~':
	case '*':
	case '\'':
	case '(':
	case ')':
		return true;
	default:
		return false;
	}
}


static bool is_unreserved(char c)
{
	return isalnum(c) || is_mark(c);
}


static bool is_user_unreserved(int c)
{
	switch (c) {

	case '&':
	case '=':
	case '+':
	case '$':
	case ',':
	case ';':
	case '?':
	case '/':
		return true;
	default:
		return false;
	}
}

static bool is_hnv_unreserved(char c)
{
	switch (c) {

	case '[':
	case ']':
	case '/':
	case '?':
	case ':':
	case '+':
	case '$':
		return true;
	default:
		return false;
	}
}


static bool is_user(char c)
{
	return is_unreserved(c) || is_user_unreserved(c);
}


static bool is_password(char c)
{
	switch (c) {

	case '&':
	case '=':
	case '+':
	case '$':
	case ',':
		return true;
	default:
		return is_unreserved(c);
	}
}


static bool is_param_unreserved(char c)
{
	switch (c) {

	case '[':
	case ']':
	case '/':
	case ':':
	case '&':
	case '+':
	case '$':
		return true;
	default:
		return false;
	}
}


static bool is_paramchar(char c)
{
	return is_param_unreserved(c) || is_unreserved(c);
}


static bool is_hvalue(char c)
{
	return is_hnv_unreserved(c) || is_unreserved(c);
}


static int comp_escape(struct re_printf *pf, const struct pl *pl, esc_h *eh)
{
	size_t i;
	int err = 0;

	if (!pf || !pl || !eh)
		return EINVAL;

	for (i=0; i<pl->l && !err; i++) {
		const char c = pl->p[i];

		if (eh(c)) {
			err = pf->vph(&c, 1, pf->arg);
		}
		else {
			err = re_hprintf(pf, "%%%02X", c);
		}
	}

	return err;
}


static int comp_unescape(struct re_printf *pf, const struct pl *pl, esc_h *eh)
{
	size_t i;
	int err = 0;

	if (!pf || !pl || !eh)
		return EINVAL;

	for (i=0; i<pl->l && !err; i++) {
		const char c = pl->p[i];

		if (eh(c)) {
			err = pf->vph(&c, 1, pf->arg);
			continue;
		}

		if ('%' == c) {
			if (i+2 < pl->l) {
				const uint8_t hi = ch_hex(pl->p[++i]);
				const uint8_t lo = ch_hex(pl->p[++i]);
				const char b = hi<<4 | lo;
				err = pf->vph(&b, 1, pf->arg);
			}
			else {
				DEBUG_WARNING("unescape: short uri (%u)\n", i);
				return EBADMSG;
			}
		}
		else {
			DEBUG_WARNING("unescape: illegal '%c' in %r\n",
				      c, pl);
			return EINVAL;
		}
	}

	return err;
}


/**
 * Escape a URI user component
 *
 * @param pf Print function
 * @param pl String to escape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_user_escape(struct re_printf *pf, const struct pl *pl)
{
	return comp_escape(pf, pl, is_user);
}


/**
 * Unescape a URI user component
 *
 * @param pf Print function
 * @param pl String to unescape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_user_unescape(struct re_printf *pf, const struct pl *pl)
{
	return comp_unescape(pf, pl, is_user);
}


/**
 * Escape a URI password component
 *
 * @param pf Print function
 * @param pl String to escape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_password_escape(struct re_printf *pf, const struct pl *pl)
{
	return comp_escape(pf, pl, is_password);
}


/**
 * Unescape a URI password component
 *
 * @param pf Print function
 * @param pl String to unescape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_password_unescape(struct re_printf *pf, const struct pl *pl)
{
	return comp_unescape(pf, pl, is_password);
}


/**
 * Escape one URI Parameter value
 *
 * @param pf Print function
 * @param pl String to escape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_param_escape(struct re_printf *pf, const struct pl *pl)
{
	return comp_escape(pf, pl, is_paramchar);
}


/**
 * Unescape one URI Parameter value
 *
 * @param pf Print function
 * @param pl String to unescape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_param_unescape(struct re_printf *pf, const struct pl *pl)
{
	return comp_unescape(pf, pl, is_paramchar);
}


/**
 * Escape one URI Header name/value
 *
 * @param pf Print function
 * @param pl String to escape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_header_escape(struct re_printf *pf, const struct pl *pl)
{
	return comp_escape(pf, pl, is_hvalue);
}


/**
 * Unescape one URI Header name/value
 *
 * @param pf Print function
 * @param pl String to unescape
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_header_unescape(struct re_printf *pf, const struct pl *pl)
{
	return comp_unescape(pf, pl, is_hvalue);
}
