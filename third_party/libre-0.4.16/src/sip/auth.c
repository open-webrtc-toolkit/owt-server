/**
 * @file sip/auth.c  SIP Authentication
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_uri.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_sys.h>
#include <re_md5.h>
#include <re_httpauth.h>
#include <re_udp.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


struct sip_auth {
	struct list realml;
	sip_auth_h *authh;
	void *arg;
	bool ref;
	int err;
};


struct realm {
	struct le le;
	char *realm;
	char *nonce;
	char *qop;
	char *opaque;
	char *user;
	char *pass;
	uint32_t nc;
	enum sip_hdrid hdr;
};


static int dummy_handler(char **user, char **pass, const char *rlm, void *arg)
{
	(void)user;
	(void)pass;
	(void)rlm;
	(void)arg;

	return EAUTH;
}


static void realm_destructor(void *arg)
{
	struct realm *realm = arg;

	list_unlink(&realm->le);
	mem_deref(realm->realm);
	mem_deref(realm->nonce);
	mem_deref(realm->qop);
	mem_deref(realm->opaque);
	mem_deref(realm->user);
	mem_deref(realm->pass);
}


static void auth_destructor(void *arg)
{
	struct sip_auth *auth = arg;

	if (auth->ref)
		mem_deref(auth->arg);

	list_flush(&auth->realml);
}


static int mkdigest(uint8_t *digest, const struct realm *realm,
		    const char *met, const char *uri, uint64_t cnonce)
{
	uint8_t ha1[MD5_SIZE], ha2[MD5_SIZE];
	int err;

	err = md5_printf(ha1, "%s:%s:%s",
			 realm->user, realm->realm, realm->pass);
	if (err)
		return err;

	err = md5_printf(ha2, "%s:%s", met, uri);
	if (err)
		return err;

	if (realm->qop)
		return md5_printf(digest, "%w:%s:%08x:%016llx:auth:%w",
				  ha1, sizeof(ha1),
				  realm->nonce,
				  realm->nc,
				  cnonce,
				  ha2, sizeof(ha2));
	else
		return md5_printf(digest, "%w:%s:%w",
				  ha1, sizeof(ha1),
				  realm->nonce,
				  ha2, sizeof(ha2));
}


static bool cmp_handler(struct le *le, void *arg)
{
	struct realm *realm = le->data;
	struct pl *chrealm = arg;

	/* handle multiple authenticate headers with equal realm value */
	if (realm->nc == 1)
		return false;

	return 0 == pl_strcasecmp(chrealm, realm->realm);
}


static bool auth_handler(const struct sip_hdr *hdr, const struct sip_msg *msg,
			 void *arg)
{
	struct httpauth_digest_chall ch;
	struct sip_auth *auth = arg;
	struct realm *realm = NULL;
	int err;
	(void)msg;

	if (httpauth_digest_challenge_decode(&ch, &hdr->val)) {
		err = EBADMSG;
		goto out;
	}

	if (pl_isset(&ch.algorithm) && pl_strcasecmp(&ch.algorithm, "md5")) {
		err = ENOSYS;
		goto out;
	}

	realm = list_ledata(list_apply(&auth->realml, true, cmp_handler,
				       &ch.realm));
	if (!realm) {
		realm = mem_zalloc(sizeof(*realm), realm_destructor);
		if (!realm) {
			err = ENOMEM;
			goto out;
		}

		list_append(&auth->realml, &realm->le, realm);

		err = pl_strdup(&realm->realm, &ch.realm);
		if (err)
			goto out;

		err = auth->authh(&realm->user, &realm->pass,
				  realm->realm, auth->arg);
		if (err)
			goto out;
	}
	else {
		if (!pl_isset(&ch.stale) || pl_strcasecmp(&ch.stale, "true")) {
			err = EAUTH;
			goto out;
		}

		realm->nonce  = mem_deref(realm->nonce);
		realm->qop    = mem_deref(realm->qop);
		realm->opaque = mem_deref(realm->opaque);
	}

	realm->hdr = hdr->id;
	realm->nc  = 1;

	err = pl_strdup(&realm->nonce, &ch.nonce);

	if (pl_isset(&ch.qop))
		err |= pl_strdup(&realm->qop, &ch.qop);

	if (pl_isset(&ch.opaque))
		err |= pl_strdup(&realm->opaque, &ch.opaque);

 out:
	if (err) {
		mem_deref(realm);
		auth->err = err;
		return true;
	}

	return false;
}


/**
 * Update a SIP authentication state from a SIP message
 *
 * @param auth SIP Authentication state
 * @param msg  SIP Message
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_auth_authenticate(struct sip_auth *auth, const struct sip_msg *msg)
{
	if (!auth || !msg)
		return EINVAL;

	if (sip_msg_hdr_apply(msg, true, SIP_HDR_WWW_AUTHENTICATE,
			      auth_handler, auth))
		return auth->err;

	if (sip_msg_hdr_apply(msg, true, SIP_HDR_PROXY_AUTHENTICATE,
			      auth_handler, auth))
		return auth->err;

	return 0;
}


int sip_auth_encode(struct mbuf *mb, struct sip_auth *auth, const char *met,
		    const char *uri)
{
	struct le *le;
	int err = 0;

	if (!mb || !auth || !met || !uri)
		return EINVAL;

	for (le = auth->realml.head; le; le = le->next) {

		const uint64_t cnonce = rand_u64();
		struct realm *realm = le->data;
		uint8_t digest[MD5_SIZE];

		err = mkdigest(digest, realm, met, uri, cnonce);
		if (err)
			break;

		switch (realm->hdr) {

		case SIP_HDR_WWW_AUTHENTICATE:
			err = mbuf_write_str(mb, "Authorization: ");
			break;

		case SIP_HDR_PROXY_AUTHENTICATE:
			err = mbuf_write_str(mb, "Proxy-Authorization: ");
			break;

		default:
			continue;
		}

		err |= mbuf_printf(mb, "Digest username=\"%s\"", realm->user);
		err |= mbuf_printf(mb, ", realm=\"%s\"", realm->realm);
		err |= mbuf_printf(mb, ", nonce=\"%s\"", realm->nonce);
		err |= mbuf_printf(mb, ", uri=\"%s\"", uri);
		err |= mbuf_printf(mb, ", response=\"%w\"",
				   digest, sizeof(digest));

		if (realm->opaque)
			err |= mbuf_printf(mb, ", opaque=\"%s\"",
					   realm->opaque);

		if (realm->qop) {
			err |= mbuf_printf(mb, ", cnonce=\"%016llx\"", cnonce);
			err |= mbuf_write_str(mb, ", qop=auth");
			err |= mbuf_printf(mb, ", nc=%08x", realm->nc);
		}

		++realm->nc;

		err |= mbuf_write_str(mb, "\r\n");
		if (err)
			break;
	}

	return err;
}


/**
 * Allocate a SIP authentication state
 *
 * @param authp Pointer to allocated SIP authentication state
 * @param authh Authentication handler
 * @param arg   Handler argument
 * @param ref   True to mem_ref() argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_auth_alloc(struct sip_auth **authp, sip_auth_h *authh,
		   void *arg, bool ref)
{
	struct sip_auth *auth;

	if (!authp)
		return EINVAL;

	auth = mem_zalloc(sizeof(*auth), auth_destructor);
	if (!auth)
		return ENOMEM;

	auth->authh = authh ? authh : dummy_handler;
	auth->arg   = ref ? mem_ref(arg) : arg;
	auth->ref   = ref;

	*authp = auth;

	return 0;
}


/**
 * Reset a SIP authentication state
 *
 * @param auth SIP Authentication state
 */
void sip_auth_reset(struct sip_auth *auth)
{
	if (!auth)
		return;

	list_flush(&auth->realml);
}
