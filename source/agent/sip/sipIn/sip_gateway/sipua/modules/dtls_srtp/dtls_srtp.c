/**
 * @file dtls_srtp.c DTLS-SRTP media encryption
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re/re.h>
#include <baresip.h>
#include <string.h>
#include "dtls_srtp.h"


/**
 * @defgroup dtls_srtp dtls_srtp
 *
 * DTLS-SRTP media encryption module
 *
 * This module implements end-to-end media encryption using DTLS-SRTP
 * which is now mandatory for WebRTC endpoints.
 *
 * DTLS-SRTP can be enabled in ~/.baresip/accounts:
 *
 \verbatim
  <sip:user@domain.com>;mediaenc=dtls_srtp
  <sip:user@domain.com>;mediaenc=dtls_srtpf
  <sip:user@domain.com>;mediaenc=srtp-mandf
 \endverbatim
 *
 *
 * Internally the protocol stack diagram looks something like this:
 *
 \verbatim
 *                    application
 *                        |
 *                        |
 *            [DTLS]   [SRTP]
 *                \      /
 *                 \    /
 *                  \  /
 *                   \/
 *              ( TURN/ICE )
 *                   |
 *                   |
 *                [socket]
 \endverbatim
 *
 */

struct menc_sess {
	struct sdp_session *sdp;
	bool offerer;
	menc_error_h *errorh;
	void *arg;
};

/* media */
struct dtls_srtp {
	struct comp compv[2];
	const struct menc_sess *sess;
	struct sdp_media *sdpm;
	struct tmr tmr;
	bool started;
	bool active;
	bool mux;
};

static struct tls *tls;
static const char* srtp_profiles =
	"SRTP_AES128_CM_SHA1_80:"
	"SRTP_AES128_CM_SHA1_32";


static void sess_destructor(void *arg)
{
	struct menc_sess *sess = arg;

	mem_deref(sess->sdp);
}


static void destructor(void *arg)
{
	struct dtls_srtp *st = arg;
	size_t i;

	tmr_cancel(&st->tmr);

	for (i=0; i<2; i++) {
		struct comp *c = &st->compv[i];

		mem_deref(c->uh_srtp);
		mem_deref(c->tls_conn);
		mem_deref(c->dtls_sock);
		mem_deref(c->app_sock);  /* must be freed last */
		mem_deref(c->tx);
		mem_deref(c->rx);
	}

	mem_deref(st->sdpm);
}


static bool verify_fingerprint(const struct sdp_session *sess,
			       const struct sdp_media *media,
			       struct tls_conn *tc)
{
	struct pl hash;
	uint8_t md_sdp[32], md_dtls[32];
	size_t sz_sdp = sizeof(md_sdp);
	size_t sz_dtls;
	enum tls_fingerprint type;
	int err;

	if (sdp_fingerprint_decode(sdp_media_session_rattr(media, sess,
							   "fingerprint"),
				   &hash, md_sdp, &sz_sdp))
		return false;

	if (0 == pl_strcasecmp(&hash, "sha-1")) {
		type = TLS_FINGERPRINT_SHA1;
		sz_dtls = 20;
	}
	else if (0 == pl_strcasecmp(&hash, "sha-256")) {
		type = TLS_FINGERPRINT_SHA256;
		sz_dtls = 32;
	}
	else {
		warning("dtls_srtp: unknown fingerprint '%r'\n", &hash);
		return false;
	}

	err = tls_peer_fingerprint(tc, type, md_dtls, sizeof(md_dtls));
	if (err) {
		warning("dtls_srtp: could not get DTLS fingerprint (%m)\n",
			err);
		return false;
	}

	if (sz_sdp != sz_dtls || 0 != memcmp(md_sdp, md_dtls, sz_sdp)) {
		warning("dtls_srtp: %r fingerprint mismatch\n", &hash);
		info("SDP:  %w\n", md_sdp, sz_sdp);
		info("DTLS: %w\n", md_dtls, sz_dtls);
		return false;
	}

	info("dtls_srtp: verified %r fingerprint OK\n", &hash);

	return true;
}


static int session_alloc(struct menc_sess **sessp,
			 struct sdp_session *sdp, bool offerer,
			 menc_error_h *errorh, void *arg)
{
	struct menc_sess *sess;
	int err;

	if (!sessp || !sdp)
		return EINVAL;

	sess = mem_zalloc(sizeof(*sess), sess_destructor);
	if (!sess)
		return ENOMEM;

	sess->sdp     = mem_ref(sdp);
	sess->offerer = offerer;
	sess->errorh  = errorh;
	sess->arg     = arg;

	/* RFC 4145 */
	err = sdp_session_set_lattr(sdp, true, "setup",
				    offerer ? "actpass" : "active");
	if (err)
		goto out;

	/* RFC 4572 */
	err = sdp_session_set_lattr(sdp, true, "fingerprint", "SHA-256 %H",
				    dtls_print_sha256_fingerprint, tls);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(sess);
	else
		*sessp = sess;

	return err;
}


static void dtls_estab_handler(void *arg)
{
	struct comp *comp = arg;
	const struct dtls_srtp *ds = comp->ds;
	enum srtp_suite suite;
	uint8_t cli_key[30], srv_key[30];
	int err;

	if (!verify_fingerprint(ds->sess->sdp, ds->sdpm, comp->tls_conn)) {
		warning("dtls_srtp: could not verify remote fingerprint\n");
		if (ds->sess->errorh)
			ds->sess->errorh(EPIPE, ds->sess->arg);
		return;
	}

	err = tls_srtp_keyinfo(comp->tls_conn, &suite,
			       cli_key, sizeof(cli_key),
			       srv_key, sizeof(srv_key));
	if (err) {
		warning("dtls_srtp: could not get SRTP keyinfo (%m)\n", err);
		return;
	}

	comp->negotiated = true;

	info("dtls_srtp: ---> DTLS-SRTP complete (%s/%s) Profile=%s\n",
	     sdp_media_name(ds->sdpm),
	     comp->is_rtp ? "RTP" : "RTCP", srtp_suite_name(suite));

	err |= srtp_stream_add(&comp->tx, suite,
			       ds->active ? cli_key : srv_key, 30, true);
	err |= srtp_stream_add(&comp->rx, suite,
			       ds->active ? srv_key : cli_key, 30, false);

	err |= srtp_install(comp);
	if (err) {
		warning("dtls_srtp: srtp_install: %m\n", err);
	}

	/* todo: notify application that crypto is up and running */
}


static void dtls_close_handler(int err, void *arg)
{
	struct comp *comp = arg;

	info("dtls_srtp: dtls-connection closed (%m)\n", err);

	comp->tls_conn = mem_deref(comp->tls_conn);

	if (!comp->negotiated) {

		if (comp->ds->sess->errorh)
			comp->ds->sess->errorh(err, comp->ds->sess->arg);
	}
}


static void dtls_conn_handler(const struct sa *peer, void *arg)
{
	struct comp *comp = arg;
	int err;
	(void)peer;

	info("dtls_srtp: incoming DTLS connect from %J\n", peer);

	err = dtls_accept(&comp->tls_conn, tls, comp->dtls_sock,
			  dtls_estab_handler, NULL, dtls_close_handler, comp);
	if (err) {
		warning("dtls_srtp: dtls_accept failed (%m)\n", err);
		return;
	}
}


static int component_start(struct comp *comp, struct sdp_media *sdpm)
{
	struct sa raddr;
	int err = 0;

	if (!comp->app_sock || comp->negotiated || comp->dtls_sock)
		return 0;

	if (comp->is_rtp)
		raddr = *sdp_media_raddr(sdpm);
	else
		sdp_media_raddr_rtcp(sdpm, &raddr);

	err = dtls_listen(&comp->dtls_sock, NULL,
			  comp->app_sock, 2, LAYER_DTLS,
			  dtls_conn_handler, comp);
	if (err) {
		warning("dtls_srtp: dtls_listen failed (%m)\n", err);
		return err;
	}

	if (sa_isset(&raddr, SA_ALL)) {

		if (comp->ds->active && !comp->tls_conn) {

			err = dtls_connect(&comp->tls_conn, tls,
					   comp->dtls_sock, &raddr,
					   dtls_estab_handler, NULL,
					   dtls_close_handler, comp);
			if (err) {
				warning("dtls_srtp: dtls_connect()"
					" failed (%m)\n", err);
				return err;
			}
		}
	}

	return err;
}


static int media_start(struct dtls_srtp *st, struct sdp_media *sdpm)
{
	int err = 0;

	if (st->started)
		return 0;

	info("dtls_srtp: media=%s -- start DTLS %s\n",
	     sdp_media_name(sdpm), st->active ? "client" : "server");

	if (!sdp_media_has_media(sdpm))
		return 0;

	err = component_start(&st->compv[0], sdpm);

	if (!st->mux)
		err |= component_start(&st->compv[1], sdpm);

	if (err)
		return err;

	st->started = true;

	return 0;
}


static void timeout(void *arg)
{
	struct dtls_srtp *st = arg;

	media_start(st, st->sdpm);
}


static int media_alloc(struct menc_media **mp, struct menc_sess *sess,
		       struct rtp_sock *rtp, int proto,
		       void *rtpsock, void *rtcpsock,
		       struct sdp_media *sdpm)
{
	struct dtls_srtp *st;
	const char *setup, *fingerprint;
	int err = 0;
	unsigned i;
	(void)rtp;

	if (!mp || !sess || proto != IPPROTO_UDP)
		return EINVAL;

	st = (struct dtls_srtp *)*mp;
	if (st)
		goto setup;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	st->sess = sess;
	st->sdpm = mem_ref(sdpm);
	st->compv[0].app_sock = mem_ref(rtpsock);
	st->compv[1].app_sock = mem_ref(rtcpsock);

	for (i=0; i<2; i++)
		st->compv[i].ds = st;

	st->compv[0].is_rtp = true;
	st->compv[1].is_rtp = false;

	err = sdp_media_set_alt_protos(st->sdpm, 4,
				       "RTP/SAVP",
				       "RTP/SAVPF",
				       "UDP/TLS/RTP/SAVP",
				       "UDP/TLS/RTP/SAVPF");
	if (err)
		goto out;

 out:
	if (err) {
		mem_deref(st);
		return err;
	}
	else
		*mp = (struct menc_media *)st;

 setup:
	st->mux = (rtpsock == rtcpsock) || (rtcpsock == NULL);

	setup = sdp_media_session_rattr(st->sdpm, st->sess->sdp, "setup");
	if (setup) {
		st->active = !(0 == str_casecmp(setup, "active"));

		/* note: we need to wait for ICE to settle ... */
		tmr_start(&st->tmr, 100, timeout, st);
	}

	/* SDP offer/answer on fingerprint attribute */
	fingerprint = sdp_media_session_rattr(st->sdpm, st->sess->sdp,
					      "fingerprint");
	if (fingerprint) {

		struct pl hash;

		err = sdp_fingerprint_decode(fingerprint, &hash, NULL, NULL);
		if (err)
			return err;

		if (0 == pl_strcasecmp(&hash, "SHA-1")) {
			err = sdp_media_set_lattr(st->sdpm, true,
						  "fingerprint", "SHA-1 %H",
						  dtls_print_sha1_fingerprint,
						  tls);
		}
		else if (0 == pl_strcasecmp(&hash, "SHA-256")) {
			err = sdp_media_set_lattr(st->sdpm, true,
						  "fingerprint", "SHA-256 %H",
						 dtls_print_sha256_fingerprint,
						  tls);
		}
		else {
			info("dtls_srtp: unsupported fingerprint hash `%r'\n",
			     &hash);
			return EPROTO;
		}
	}

	return err;
}


static struct menc dtls_srtp = {
	LE_INIT, "dtls_srtp",  "UDP/TLS/RTP/SAVP", session_alloc, media_alloc
};

static struct menc dtls_srtpf = {
	LE_INIT, "dtls_srtpf", "UDP/TLS/RTP/SAVPF", session_alloc, media_alloc
};

static struct menc dtls_srtp2 = {
	/* note: temp for Webrtc interop */
	LE_INIT, "srtp-mandf", "RTP/SAVPF", session_alloc, media_alloc
};


static int module_init(void)
{
	int err;

	err = tls_alloc(&tls, TLS_METHOD_DTLSV1, NULL, NULL);
	if (err) {
		warning("dtls_srtp: failed to create DTLS context (%m)\n",
			err);
		return err;
	}

	err = tls_set_selfsigned(tls, "dtls@baresip");
	if (err) {
		warning("dtls_srtp: failed to self-sign certificate (%m)\n",
			err);
		return err;
	}

	tls_set_verify_client(tls);

	err = tls_set_srtp(tls, srtp_profiles);
	if (err) {
		warning("dtls_srtp: failed to enable SRTP profile (%m)\n",
			err);
		return err;
	}

	menc_register(&dtls_srtpf);
	menc_register(&dtls_srtp);
	menc_register(&dtls_srtp2);

	debug("DTLS-SRTP ready with profiles %s\n", srtp_profiles);

	return 0;
}


static int module_close(void)
{
	menc_unregister(&dtls_srtp);
	menc_unregister(&dtls_srtpf);
	menc_unregister(&dtls_srtp2);
	tls = mem_deref(tls);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(dtls_srtp) = {
	"dtls_srtp",
	"menc",
	module_init,
	module_close
};
