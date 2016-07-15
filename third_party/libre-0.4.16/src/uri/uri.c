/**
 * @file uri.c  Uniform Resource Identifier (URI) module
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_uri.h>


/**
 * Encode a URI object
 *
 * @param pf  Print function to encode into
 * @param uri URI object
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_encode(struct re_printf *pf, const struct uri *uri)
{
	int err;

	if (!uri)
		return 0;

	if (!pl_isset(&uri->scheme) || !pl_isset(&uri->host))
		return EINVAL;

	err = re_hprintf(pf, "%r:", &uri->scheme);
	if (err)
		return err;

	if (pl_isset(&uri->user)) {
		err = re_hprintf(pf, "%r", &uri->user);

		if (pl_isset(&uri->password))
			err |= re_hprintf(pf, ":%r", &uri->password);

		err |= pf->vph("@", 1, pf->arg);

		if (err)
			return err;
	}

	/* The IPv6 address is delimited by '[' and ']' */
	switch (uri->af) {

#ifdef HAVE_INET6
	case AF_INET6:
		err = re_hprintf(pf, "[%r]", &uri->host);
		break;
#endif

	default:
		err = re_hprintf(pf, "%r", &uri->host);
		break;
	}
	if (err)
		return err;

	if (uri->port)
		err = re_hprintf(pf, ":%u", uri->port);

	err |= re_hprintf(pf, "%r%r", &uri->params, &uri->headers);

	return err;
}


/**
 * Decode host-port portion of a URI (if present)
 *
 * @return 0 if success, otherwise errorcode
 */
static int decode_hostport(const struct pl *hostport, struct pl *host,
			   struct pl *port)
{
	/* Try IPv6 first */
	if (!re_regex(hostport->p, hostport->l, "\\[[0-9a-f:]+\\][:]*[0-9]*",
		      host, NULL, port))
		return 0;

	/* Then non-IPv6 host */
	return re_regex(hostport->p, hostport->l, "[^:]+[:]*[0-9]*",
			host, NULL, port);
}


/**
 * Decode a pointer-length object into a URI object
 *
 * @param uri  URI object
 * @param pl   Pointer-length object to decode from
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_decode(struct uri *uri, const struct pl *pl)
{
	struct sa addr;
	struct pl port = PL_INIT;
	struct pl hostport;
	int err;

	if (!uri || !pl)
		return EINVAL;

	memset(uri, 0, sizeof(*uri));
	if (0 == re_regex(pl->p, pl->l,
			  "[^:]+:[^@:]*[:]*[^@]*@[^;? ]+[^?]*[^]*",
			  &uri->scheme, &uri->user, NULL, &uri->password,
			  &hostport, &uri->params, &uri->headers)) {

		if (0 == decode_hostport(&hostport, &uri->host, &port))
			goto out;
	}

	memset(uri, 0, sizeof(*uri));
	err = re_regex(pl->p, pl->l, "[^:]+:[^;? ]+[^?]*[^]*",
		       &uri->scheme, &hostport, &uri->params, &uri->headers);
	if (0 == err) {
		err = decode_hostport(&hostport, &uri->host, &port);
		if (0 == err)
			goto out;
	}

	return err;

 out:
	/* Cache host address family */
	if (0 == sa_set(&addr, &uri->host, 0))
		uri->af = sa_af(&addr);
	else
		uri->af = AF_UNSPEC;

	if (pl_isset(&port))
		uri->port = (uint16_t)pl_u32(&port);

	return 0;
}


/**
 * Get a URI parameter and possibly the value of it
 *
 * @param pl     Pointer-length string containing parameters
 * @param pname  URI Parameter name
 * @param pvalue Returned URI Parameter value
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_param_get(const struct pl *pl, const struct pl *pname,
		  struct pl *pvalue)
{
	char expr[128];

	if (!pl || !pname || !pvalue)
		return EINVAL;

	(void)re_snprintf(expr, sizeof(expr), ";%r[=]*[^;]*", pname);

	return re_regex(pl->p, pl->l, expr, NULL, pvalue);
}


/**
 * Call the apply handler for each URI Parameter
 *
 * @param pl  Pointer-length string containing parameters
 * @param ah  Apply handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode (returned from handler)
 */
int uri_params_apply(const struct pl *pl, uri_apply_h *ah, void *arg)
{
	struct pl plr, pname, eq, pvalue;
	int err = 0;

	if (!pl || !ah)
		return EINVAL;

	plr = *pl;

	while (plr.l > 0) {

		err = re_regex(plr.p, plr.l, ";[^;=]+[=]*[^;]*",
			       &pname, &eq, &pvalue);
		if (err)
			break;

		pl_advance(&plr, 1 + pname.l + eq.l + pvalue.l);

		err = ah(&pname, &pvalue, arg);
		if (err)
			break;
	}

	return err;
}


/**
 * Get a URI header and possibly the value of it
 *
 * @param pl     Pointer-length string containing URI Headers
 * @param hname  URI Header name
 * @param hvalue Returned URI Header value
 *
 * @return 0 if success, otherwise errorcode
 */
int uri_header_get(const struct pl *pl, const struct pl *hname,
		   struct pl *hvalue)
{
	char expr[128];

	if (!pl || !hname || !hvalue)
		return EINVAL;

	(void)re_snprintf(expr, sizeof(expr), "[?&]1%r=[^&]+", hname);

	return re_regex(pl->p, pl->l, expr, NULL, hvalue);
}


/**
 * Call the apply handler for each URI Header
 *
 * @param pl  Pointer-length string containing URI Headers
 * @param ah  Apply handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode (returned from handler)
 */
int uri_headers_apply(const struct pl *pl, uri_apply_h *ah, void *arg)
{
	struct pl plr, sep, hname, hvalue;
	int err = 0;

	if (!pl || !ah)
		return EINVAL;

	plr = *pl;

	while (plr.l > 0) {

		err = re_regex(plr.p, plr.l, "[?&]1[^=]+=[^&]+",
			       &sep, &hname, &hvalue);
		if (err)
			break;

		pl_advance(&plr, sep.l + hname.l + 1 + hvalue.l);

		err = ah(&hname, &hvalue, arg);
		if (err)
			break;
	}

	return err;
}
