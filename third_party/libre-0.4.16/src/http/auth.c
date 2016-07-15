/**
 * @file http/auth.c HTTP Authentication
 *
 * Copyright (C) 2011 Creytiv.com
 */

#include <string.h>
#include <time.h>
#include <re_types.h>
#include <re_sys.h>
#include <re_md5.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_msg.h>
#include <re_httpauth.h>
#include <re_http.h>


enum {
	NONCE_EXPIRES  = 300,
	NONCE_MIN_SIZE = 33,
};


static uint64_t secret;
static bool secret_set;


/**
 * Print HTTP digest authentication challenge
 *
 * @param pf   Print function for output
 * @param auth Authentication parameteres
 *
 * @return 0 if success, otherwise errorcode
 */
int http_auth_print_challenge(struct re_printf *pf,
			      const struct http_auth *auth)
{
	uint8_t key[MD5_SIZE];
	uint64_t nv[2];

	if (!auth)
		return 0;

	if (!secret_set) {
		secret = rand_u64();
		secret_set = true;
	}

	nv[0] = time(NULL);
	nv[1] = secret;

	md5((uint8_t *)nv, sizeof(nv), key);

	return re_hprintf(pf,
			  "Digest realm=\"%s\", nonce=\"%w%llx\", "
			  "qop=\"auth\"%s",
			  auth->realm,
			  key, sizeof(key), nv[0],
			  auth->stale ? ", stale=true" : "");
}


static int chk_nonce(const struct pl *nonce, uint32_t expires)
{
	uint8_t nkey[MD5_SIZE], ckey[MD5_SIZE];
	uint64_t nv[2];
	struct pl pl;
	int64_t age;
	unsigned i;

	if (!nonce || !nonce->p || nonce->l < NONCE_MIN_SIZE)
		return EINVAL;

	pl = *nonce;

	for (i=0; i<sizeof(nkey); i++) {
		nkey[i]  = ch_hex(*pl.p++) << 4;
		nkey[i] += ch_hex(*pl.p++);
		pl.l -= 2;
	}

	nv[0] = pl_x64(&pl);
	nv[1] = secret;

	md5((uint8_t *)nv, sizeof(nv), ckey);

	if (memcmp(nkey, ckey, MD5_SIZE))
		return EAUTH;

	age = time(NULL) - nv[0];

	if (age < 0 || age > expires)
		return ETIMEDOUT;

	return 0;
}


/**
 * Check HTTP digest authorization
 *
 * @param hval   Authorization header value
 * @param method Request method
 * @param auth   Authentication parameteres
 * @param authh  Authentication handler
 * @param arg    Authentication handler argument
 *
 * @return true if check is passed, otherwise false
 */
bool http_auth_check(const struct pl *hval, const struct pl *method,
		     struct http_auth *auth, http_auth_h *authh, void *arg)
{
	struct httpauth_digest_resp resp;
	uint8_t ha1[MD5_SIZE];

	if (!hval || !method || !auth || !authh)
		return false;

	if (httpauth_digest_response_decode(&resp, hval))
		return false;

	if (pl_strcasecmp(&resp.realm, auth->realm))
		return false;

	if (chk_nonce(&resp.nonce, NONCE_EXPIRES)) {
		auth->stale = true;
		return false;
	}

	if (authh(&resp.username, ha1, arg))
		return false;

	if (httpauth_digest_response_auth(&resp, method, ha1))
		return false;

	return true;
}


/**
 * Check HTTP digest authorization of an HTTP request
 *
 * @param msg   HTTP message
 * @param auth  Authentication parameteres
 * @param authh Authentication handler
 * @param arg   Authentication handler argument
 *
 * @return true if check is passed, otherwise false
 */
bool http_auth_check_request(const struct http_msg *msg,
			     struct http_auth *auth,
			     http_auth_h *authh, void *arg)
{
	const struct http_hdr *hdr;

	if (!msg)
		return false;

	hdr = http_msg_hdr(msg, HTTP_HDR_AUTHORIZATION);
	if (!hdr)
		return false;

	return http_auth_check(&hdr->val, &msg->met, auth, authh, arg);
}
