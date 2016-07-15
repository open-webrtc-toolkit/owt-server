/**
 * @file param.c  SIP Parameter decode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_msg.h>


/**
 * Check if a parameter exists
 *
 * @param pl   Pointer-length string
 * @param name Parameter name
 * @param val  Returned parameter value
 *
 * @return 0 for success, otherwise errorcode
 */
int msg_param_exists(const struct pl *pl, const char *name, struct pl *val)
{
	struct pl v1, v2;
	char xpr[128];

	if (!pl || !name || !val)
		return EINVAL;

	(void)re_snprintf(xpr, sizeof(xpr), ";[ \t\r\n]*%s[ \t\r\n;=]*", name);

	if (re_regex(pl->p, pl->l, xpr, &v1, &v2))
		return ENOENT;

	if (!v2.l && v2.p < pl->p + pl->l)
		return ENOENT;

	val->p = v1.p - 1;
	val->l = v2.p - val->p;

	return 0;
}


/**
 * Decode a Parameter
 *
 * @param pl   Pointer-length string
 * @param name Parameter name
 * @param val  Returned parameter value
 *
 * @return 0 for success, otherwise errorcode
 */
int msg_param_decode(const struct pl *pl, const char *name, struct pl *val)
{
	char expr[128];
	struct pl v;

	if (!pl || !name || !val)
		return EINVAL;

	(void)re_snprintf(expr, sizeof(expr),
			  ";[ \t\r\n]*%s[ \t\r\n]*=[ \t\r\n]*[~ \t\r\n;]+",
			  name);

	if (re_regex(pl->p, pl->l, expr, NULL, NULL, NULL, &v))
		return ENOENT;

	*val = v;

	return 0;
}
