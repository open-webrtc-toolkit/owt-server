/**
 * @file prm.c Generic parameter decoding
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>


/**
 * Check if a semicolon separated parameter is present
 *
 * @param pl    PL string to search
 * @param pname Parameter name
 *
 * @return true if found, false if not found
 */
bool fmt_param_exists(const struct pl *pl, const char *pname)
{
	struct pl semi, eop;
	char expr[128];

	if (!pl || !pname)
		return false;

	(void)re_snprintf(expr, sizeof(expr),
			  "[;]*[ \t\r\n]*%s[ \t\r\n;=]*",
			  pname);

	if (re_regex(pl->p, pl->l, expr, &semi, NULL, &eop))
		return false;

	if (!eop.l && eop.p < pl->p + pl->l)
		return false;

	return semi.l > 0 || pl->p == semi.p;
}


/**
 * Fetch a semicolon separated parameter from a PL string
 *
 * @param pl    PL string to search
 * @param pname Parameter name
 * @param val   Parameter value, set on return
 *
 * @return true if found, false if not found
 */
bool fmt_param_get(const struct pl *pl, const char *pname, struct pl *val)
{
	struct pl semi;
	char expr[128];

	if (!pl || !pname)
		return false;

	(void)re_snprintf(expr, sizeof(expr),
			  "[;]*[ \t\r\n]*%s[ \t\r\n]*=[ \t\r\n]*[~ \t\r\n;]+",
			  pname);

	if (re_regex(pl->p, pl->l, expr, &semi, NULL, NULL, NULL, val))
		return false;

	return semi.l > 0 || pl->p == semi.p;
}


/**
 * Apply a function handler for each semicolon separated parameter
 *
 * @param pl  PL string to search
 * @param ph  Parameter handler
 * @param arg Handler argument
 */
void fmt_param_apply(const struct pl *pl, fmt_param_h *ph, void *arg)
{
	struct pl prmv, prm, semi, name, val;

	if (!pl || !ph)
		return;

	prmv = *pl;

	while (!re_regex(prmv.p, prmv.l, "[ \t\r\n]*[~;]+[;]*",
			 NULL, &prm, &semi)) {

		pl_advance(&prmv, semi.p + semi.l - prmv.p);

		if (re_regex(prm.p, prm.l,
			     "[^ \t\r\n=]+[ \t\r\n]*[=]*[ \t\r\n]*[~ \t\r\n]*",
			     &name, NULL, NULL, NULL, &val))
			break;

		ph(&name, &val, arg);
	}
}
