/**
 * @file re_httpauth.h  Interface to HTTP Authentication
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** HTTP Digest Challenge */
struct httpauth_digest_chall {
	struct pl realm;
	struct pl nonce;

	/* optional */
	struct pl opaque;
	struct pl stale;
	struct pl algorithm;
	struct pl qop;
};

/** HTTP Digest response */
struct httpauth_digest_resp {
	struct pl realm;
	struct pl nonce;
	struct pl response;
	struct pl username;
	struct pl uri;

	/* optional */
	struct pl nc;
	struct pl cnonce;
	struct pl qop;
};


int httpauth_digest_challenge_decode(struct httpauth_digest_chall *chall,
				     const struct pl *hval);
int httpauth_digest_response_decode(struct httpauth_digest_resp *resp,
				    const struct pl *hval);
int httpauth_digest_response_auth(const struct httpauth_digest_resp *resp,
				  const struct pl *method, const uint8_t *ha1);
