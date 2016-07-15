/**
 * @file srtcp.c  Secure Real-time Transport Control Protocol (SRTCP)
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_hmac.h>
#include <re_sha.h>
#include <re_aes.h>
#include <re_net.h>
#include <re_srtp.h>
#include "srtp.h"


static int get_rtcp_ssrc(uint32_t *ssrc, struct mbuf *mb)
{
	if (mbuf_get_left(mb) < 8)
		return EBADMSG;

	mbuf_advance(mb, 4);
	*ssrc = ntohl(mbuf_read_u32(mb));

	return 0;
}


int srtcp_encrypt(struct srtp *srtp, struct mbuf *mb)
{
	struct srtp_stream *strm;
	struct comp *rtcp;
	uint32_t ssrc;
	size_t start;
	int ep = 0;
	int err;

	if (!srtp || !mb)
		return EINVAL;

	rtcp = &srtp->rtcp;
	start = mb->pos;

	err = get_rtcp_ssrc(&ssrc, mb);
	if (err)
		return err;

	err = stream_get(&strm, srtp, ssrc);
	if (err)
		return err;

	strm->rtcp_index = (strm->rtcp_index+1) & 0x7fffffff;

	if (rtcp->aes) {
		union vect128 iv;
		uint8_t *p = mbuf_buf(mb);

		srtp_iv_calc(&iv, &rtcp->k_s, ssrc, strm->rtcp_index);

		aes_set_iv(rtcp->aes, iv.u8);
		err = aes_encr(rtcp->aes, p, p, mbuf_get_left(mb));
		if (err)
			return err;

		ep = 1;
	}

	/* append E-bit and SRTCP-index */
	mb->pos = mb->end;
	err = mbuf_write_u32(mb, htonl(ep<<31 | strm->rtcp_index));
	if (err)
		return err;

	if (rtcp->hmac) {
		uint8_t tag[SHA_DIGEST_LENGTH];

		mb->pos = start;

		err = hmac_digest(rtcp->hmac, tag, sizeof(tag),
				  mbuf_buf(mb), mbuf_get_left(mb));
		if (err)
			return err;

		mb->pos = mb->end;

		err = mbuf_write_mem(mb, tag, rtcp->tag_len);
		if (err)
			return err;
	}

	mb->pos = start;

	return 0;
}


int srtcp_decrypt(struct srtp *srtp, struct mbuf *mb)
{
	size_t start, eix_start, pld_start;
	struct srtp_stream *strm;
	struct comp *rtcp;
	uint32_t v, ix;
	uint32_t ssrc;
	bool ep;
	int err;

	if (!srtp || !mb)
		return EINVAL;

	rtcp = &srtp->rtcp;
	start = mb->pos;

	err = get_rtcp_ssrc(&ssrc, mb);
	if (err)
		return err;

	err = stream_get(&strm, srtp, ssrc);
	if (err)
		return err;

	pld_start = mb->pos;

	if (mbuf_get_left(mb) < (4 + rtcp->tag_len))
		return EBADMSG;

	/* Read out E-Bit, SRTCP-index and Authentication Tag */
	eix_start = mb->end - (4 + rtcp->tag_len);
	mb->pos = eix_start;
	v = ntohl(mbuf_read_u32(mb));

	ep = (v >> 31) & 1;
	ix = v & 0x7fffffff;

	if (rtcp->hmac) {
		uint8_t tag[SHA_DIGEST_LENGTH], tag_pkt[SHA_DIGEST_LENGTH];
		const size_t tag_start = mb->pos;

		err = mbuf_read_mem(mb, tag_pkt, rtcp->tag_len);
		if (err)
			return err;

		mb->pos = start;
		mb->end = tag_start;

		err = hmac_digest(rtcp->hmac, tag, sizeof(tag),
				  mbuf_buf(mb), mbuf_get_left(mb));
		if (err)
			return err;

		if (0 != memcmp(tag, tag_pkt, rtcp->tag_len))
			return EAUTH;

		/*
		 * SRTCP replay protection is as defined in Section 3.3.2,
		 * but using the SRTCP index as the index i and a separate
		 * Replay List that is specific to SRTCP.
		 */
		if (!srtp_replay_check(&strm->replay_rtcp, ix))
			return EALREADY;
	}

	mb->end = eix_start;

	if (rtcp->aes && ep) {
		union vect128 iv;
		uint8_t *p;

		mb->pos = pld_start;
		p = mbuf_buf(mb);

		srtp_iv_calc(&iv, &rtcp->k_s, ssrc, ix);

		aes_set_iv(rtcp->aes, iv.u8);
		err = aes_decr(rtcp->aes, p, p, mbuf_get_left(mb));
		if (err)
			return err;
	}

	mb->pos = start;

	return 0;
}
