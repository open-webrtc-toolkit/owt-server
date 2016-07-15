/**
 * @file ucmp.c  URI comparison
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_uri.h>


static int param_handler(const struct pl *pname, const struct pl *pvalue,
			 void *arg)
{
	struct pl *other_params = arg;
	struct pl other_pvalue = PL_INIT;
	bool both;

	if (0 == pl_strcmp(pname, "user"))
		both = true;
	else if (0 == pl_strcmp(pname, "ttl"))
		both = true;
	else if (0 == pl_strcmp(pname, "method"))
		both = true;
	else if (0 == pl_strcmp(pname, "maddr"))
		both = true;
	else if (0 == pl_strcmp(pname, "transport"))
		both = true;
	else
		both = false;

	if (uri_param_get(other_params, pname, &other_pvalue))
		return both ? ENOENT : 0;

	return pl_casecmp(pvalue, &other_pvalue);
}


static int header_handler(const struct pl *hname, const struct pl *hvalue,
			  void *arg)
{
	struct pl *other_headers = arg;
	struct pl other_hvalue;
	int err;

	err = uri_header_get(other_headers, hname, &other_hvalue);
	if (err)
		return err;

	return pl_casecmp(hvalue, &other_hvalue);
}


/**
 * Compare two URIs - see RFC 3261 Section 19.1.4
 *
 * @param l  Left-hand URI object
 * @param r  Right-hand URI object
 *
 * @return true if match, otherwise false
 */
bool uri_cmp(const struct uri *l, const struct uri *r)
{
	int err;

	if (!l || !r)
		return false;

	if (l == r)
		return true;

	/* A SIP and SIPS URI are never equivalent. */
	if (pl_casecmp(&l->scheme, &r->scheme))
		return false;

	/* Comparison of the userinfo of SIP and SIPS URIs is case-sensitive */
	if (pl_cmp(&l->user, &r->user))
		return false;

	if (pl_cmp(&l->password, &r->password))
		return false;

	if (pl_casecmp(&l->host, &r->host))
		return false;
	if (l->af != r->af)
		return false;

	if (l->port != r->port)
		return false;

	/* URI parameters */
	err = uri_params_apply(&l->params, param_handler, (void *)&r->params);
	if (err)
		return false;
	err = uri_params_apply(&r->params, param_handler, (void *)&l->params);
	if (err)
		return false;

	/* URI headers */
	err = uri_headers_apply(&l->headers, header_handler,
				(void *)&r->headers);
	if (err)
		return false;
	err = uri_headers_apply(&r->headers, header_handler,
				(void *)&l->headers);
	if (err)
		return false;

	/* Match */
	return true;
}
