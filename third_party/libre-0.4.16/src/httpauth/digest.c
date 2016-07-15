/**
 * @file digest.c  HTTP Digest authentication (RFC 2617)
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_md5.h>
#include <re_sys.h>
#include <re_httpauth.h>


typedef void (digest_decode_h)(const struct pl *name, const struct pl *val,
			       void *arg);


static const struct pl param_algorithm = PL("algorithm");
static const struct pl param_cnonce    = PL("cnonce");
static const struct pl param_nc        = PL("nc");
static const struct pl param_nonce     = PL("nonce");
static const struct pl param_opaque    = PL("opaque");
static const struct pl param_qop       = PL("qop");
static const struct pl param_realm     = PL("realm");
static const struct pl param_response  = PL("response");
static const struct pl param_uri       = PL("uri");
static const struct pl param_username  = PL("username");
static const struct pl param_stale     = PL("stale");


static void challenge_decode(const struct pl *name, const struct pl *val,
			     void *arg)
{
	struct httpauth_digest_chall *chall = arg;

	if (!pl_casecmp(name, &param_realm))
		chall->realm = *val;
	else if (!pl_casecmp(name, &param_nonce))
		chall->nonce = *val;
	else if (!pl_casecmp(name, &param_opaque))
		chall->opaque= *val;
	else if (!pl_casecmp(name, &param_stale))
		chall->stale = *val;
	else if (!pl_casecmp(name, &param_algorithm))
		chall->algorithm = *val;
	else if (!pl_casecmp(name, &param_qop))
		chall->qop = *val;
}


static void response_decode(const struct pl *name, const struct pl *val,
			    void *arg)
{
	struct httpauth_digest_resp *resp = arg;

	if (!pl_casecmp(name, &param_realm))
		resp->realm = *val;
	else if (!pl_casecmp(name, &param_nonce))
		resp->nonce = *val;
	else if (!pl_casecmp(name, &param_response))
		resp->response = *val;
	else if (!pl_casecmp(name, &param_username))
		resp->username = *val;
	else if (!pl_casecmp(name, &param_uri))
		resp->uri = *val;
	else if (!pl_casecmp(name, &param_nc))
		resp->nc = *val;
	else if (!pl_casecmp(name, &param_cnonce))
		resp->cnonce = *val;
	else if (!pl_casecmp(name, &param_qop))
		resp->qop = *val;
}


static int digest_decode(const struct pl *hval, digest_decode_h *dech,
			 void *arg)
{
	struct pl r = *hval, start, end, name, val;

	if (re_regex(r.p, r.l, "[ \t\r\n]*Digest[ \t\r\n]+", &start, &end) ||
	    start.p != r.p)
		return EBADMSG;

	pl_advance(&r, end.p - r.p);

	while (!re_regex(r.p, r.l,
			 "[ \t\r\n,]+[a-z]+[ \t\r\n]*=[ \t\r\n]*[~ \t\r\n,]*",
			 NULL, &name, NULL, NULL, &val)) {

		pl_advance(&r, val.p + val.l - r.p);

		dech(&name, &val, arg);
	}

	return 0;
}


/**
 * Decode a Digest challenge
 *
 * @param chall Digest challenge object to decode into
 * @param hval  Header value to decode from
 *
 * @return 0 if successfully decoded, otherwise errorcode
 */
int httpauth_digest_challenge_decode(struct httpauth_digest_chall *chall,
				     const struct pl *hval)
{
	int err;

	if (!chall || !hval)
		return EINVAL;

	memset(chall, 0, sizeof(*chall));

	err = digest_decode(hval, challenge_decode, chall);
	if (err)
		return err;

	if (!chall->realm.p || !chall->nonce.p)
		return EBADMSG;

	return 0;
}


/**
 * Decode a Digest response
 *
 * @param resp Digest response object to decode into
 * @param hval Header value to decode from
 *
 * @return 0 if successfully decoded, otherwise errorcode
 */
int httpauth_digest_response_decode(struct httpauth_digest_resp *resp,
				    const struct pl *hval)
{
	int err;

	if (!resp || !hval)
		return EINVAL;

	memset(resp, 0, sizeof(*resp));

	err = digest_decode(hval, response_decode, resp);
	if (err)
		return err;

	if (!resp->realm.p    ||
	    !resp->nonce.p    ||
	    !resp->response.p ||
	    !resp->username.p ||
	    !resp->uri.p)
		return EBADMSG;

	return 0;
}


/**
 * Authenticate a digest response
 *
 * @param resp   Digest response
 * @param method Request method
 * @param ha1    HA1 value from MD5(username:realm:password)
 *
 * @return 0 if successfully authenticated, otherwise errorcode
 */
int httpauth_digest_response_auth(const struct httpauth_digest_resp *resp,
				  const struct pl *method, const uint8_t *ha1)
{
	uint8_t ha2[MD5_SIZE], digest[MD5_SIZE], response[MD5_SIZE];
	const char *p;
	uint32_t i;
	int err;

	if (!resp || !method || !ha1)
		return EINVAL;

	if (resp->response.l != 32)
		return EAUTH;

	err = md5_printf(ha2, "%r:%r", method, &resp->uri);
	if (err)
		return err;

	if (pl_isset(&resp->qop))
		err = md5_printf(digest, "%w:%r:%r:%r:%r:%w",
				 ha1, (size_t)MD5_SIZE,
				 &resp->nonce,
				 &resp->nc,
				 &resp->cnonce,
				 &resp->qop,
				 ha2, sizeof(ha2));
	else
		err = md5_printf(digest, "%w:%r:%w",
				 ha1, (size_t)MD5_SIZE,
				 &resp->nonce,
				 ha2, sizeof(ha2));
	if (err)
		return err;

	for (i=0, p=resp->response.p; i<sizeof(response); i++) {
		response[i]  = ch_hex(*p++) << 4;
		response[i] += ch_hex(*p++);
	}

	if (memcmp(digest, response, MD5_SIZE))
		return EAUTH;

	return 0;
}
