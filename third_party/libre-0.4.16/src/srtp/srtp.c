/**
 * @file srtp.c  Secure Real-time Transport Protocol (SRTP)
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_hmac.h>
#include <re_sha.h>
#include <re_aes.h>
#include <re_sa.h>
#include <re_rtp.h>
#include <re_srtp.h>
#include "srtp.h"


/** SRTP protocol values */
enum {
	MAX_KEYLEN  = 32,  /**< Maximum keylength in bytes     */
};


static inline int seq_diff(uint16_t x, uint16_t y)
{
	return (int)y - (int)x;
}


static int comp_init(struct comp *c, unsigned offs,
		     const uint8_t *key, size_t key_b,
		     const uint8_t *s, size_t s_b,
		     size_t tag_len, bool encrypted)
{
	uint8_t k_e[MAX_KEYLEN], k_a[SHA_DIGEST_LENGTH];
	int err = 0;

	if (key_b > sizeof(k_e))
		return EINVAL;

	if (tag_len > SHA_DIGEST_LENGTH)
		return EINVAL;

	c->tag_len = tag_len;

	err |= srtp_derive(k_e, key_b,       0x00+offs, key, key_b, s, s_b);
	err |= srtp_derive(k_a, sizeof(k_a), 0x01+offs, key, key_b, s, s_b);
	err |= srtp_derive(c->k_s.u8, 14,    0x02+offs, key, key_b, s, s_b);
	if (err)
		return err;

	if (encrypted) {
		err = aes_alloc(&c->aes, AES_MODE_CTR, k_e, key_b*8, NULL);
		if (err)
			return err;
	}

	err = hmac_create(&c->hmac, HMAC_HASH_SHA1, k_a, sizeof(k_a));
	if (err)
		return err;

	return err;
}


static void destructor(void *arg)
{
	struct srtp *srtp = arg;

	mem_deref(srtp->rtp.aes);
	mem_deref(srtp->rtcp.aes);
	mem_deref(srtp->rtp.hmac);
	mem_deref(srtp->rtcp.hmac);

	list_flush(&srtp->streaml);
}


int srtp_alloc(struct srtp **srtpp, enum srtp_suite suite,
	       const uint8_t *key, size_t key_bytes, int flags)
{
	struct srtp *srtp;
	const uint8_t *master_salt;
	size_t cipher_bytes, auth_bytes;
	int err = 0;

	if (!srtpp || !key)
		return EINVAL;

	switch (suite) {

	case SRTP_AES_CM_128_HMAC_SHA1_80:
		cipher_bytes = 16;
		auth_bytes   = 10;
		break;

	case SRTP_AES_CM_128_HMAC_SHA1_32:
		cipher_bytes = 16;
		auth_bytes   =  4;
		break;

	case SRTP_AES_256_CM_HMAC_SHA1_80:
		cipher_bytes = 32;
		auth_bytes   = 10;
		break;

	case SRTP_AES_256_CM_HMAC_SHA1_32:
		cipher_bytes = 32;
		auth_bytes   =  4;
		break;

	default:
		return ENOTSUP;
	};

	if ((cipher_bytes + SRTP_SALT_SIZE) != key_bytes)
		return EINVAL;

	master_salt = &key[cipher_bytes];

	srtp = mem_zalloc(sizeof(*srtp), destructor);
	if (!srtp)
		return ENOMEM;

	err |= comp_init(&srtp->rtp,  0, key, cipher_bytes,
			 master_salt, SRTP_SALT_SIZE, auth_bytes, true);
	err |= comp_init(&srtp->rtcp, 3, key, cipher_bytes,
			 master_salt, SRTP_SALT_SIZE, auth_bytes,
			 !(flags & SRTP_UNENCRYPTED_SRTCP));
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(srtp);
	else
		*srtpp = srtp;

	return err;
}


int srtp_encrypt(struct srtp *srtp, struct mbuf *mb)
{
	struct srtp_stream *strm;
	struct rtp_header hdr;
	struct comp *comp;
	size_t start;
	uint64_t ix;
	int err;

	if (!srtp || !mb)
		return EINVAL;

	comp = &srtp->rtp;

	start = mb->pos;

	err = rtp_hdr_decode(&hdr, mb);
	if (err)
		return err;

	err = stream_get_seq(&strm, srtp, hdr.ssrc, hdr.seq);
	if (err)
		return err;

	/* Roll-Over Counter (ROC) */
	if (seq_diff(strm->s_l, hdr.seq) <= -32768) {
		strm->roc++;
		strm->s_l = 0;
	}

	ix = 65536ULL * strm->roc + hdr.seq;

	if (comp->aes) {
		union vect128 iv;
		uint8_t *p = mbuf_buf(mb);

		srtp_iv_calc(&iv, &comp->k_s, strm->ssrc, ix);

		aes_set_iv(comp->aes, iv.u8);
		err = aes_encr(comp->aes, p, p, mbuf_get_left(mb));
		if (err)
			return err;
	}

	if (comp->hmac) {
		const size_t tag_start = mb->end;
		uint8_t tag[SHA_DIGEST_LENGTH];

		mb->pos = tag_start;

		err = mbuf_write_u32(mb, htonl(strm->roc));
		if (err)
			return err;

		mb->pos = start;

		err = hmac_digest(comp->hmac, tag, sizeof(tag),
				  mbuf_buf(mb), mbuf_get_left(mb));
		if (err)
			return err;

		mb->pos = mb->end = tag_start;

		err = mbuf_write_mem(mb, tag, comp->tag_len);
		if (err)
			return err;
	}

	if (hdr.seq > strm->s_l)
		strm->s_l = hdr.seq;

	mb->pos = start;

	return 0;
}


int srtp_decrypt(struct srtp *srtp, struct mbuf *mb)
{
	struct srtp_stream *strm;
	struct rtp_header hdr;
	struct comp *comp;
	uint64_t ix;
	size_t start;
	int diff;
	int err;

	if (!srtp || !mb)
		return EINVAL;

	comp = &srtp->rtp;

	start = mb->pos;

	err = rtp_hdr_decode(&hdr, mb);
	if (err)
		return err;

	err = stream_get_seq(&strm, srtp, hdr.ssrc, hdr.seq);
	if (err)
		return err;

	diff = seq_diff(strm->s_l, hdr.seq);
	if (diff > 32768)
		return ETIMEDOUT;

	/* Roll-Over Counter (ROC) */
	if (diff <= -32768) {
		strm->roc++;
		strm->s_l = 0;
	}

	ix = srtp_get_index(strm->roc, strm->s_l, hdr.seq);

	if (comp->hmac) {
		uint8_t tag_calc[SHA_DIGEST_LENGTH];
		uint8_t tag_pkt[SHA_DIGEST_LENGTH];
		size_t pld_start, tag_start;

		if (mbuf_get_left(mb) < comp->tag_len)
			return EBADMSG;

		pld_start = mb->pos;
		tag_start = mb->end - comp->tag_len;

		mb->pos = tag_start;

		err = mbuf_read_mem(mb, tag_pkt, comp->tag_len);
		if (err)
			return err;

		mb->pos = mb->end = tag_start;

		err = mbuf_write_u32(mb, htonl(strm->roc));
		if (err)
			return err;

		mb->pos = start;

		err = hmac_digest(comp->hmac, tag_calc, sizeof(tag_calc),
				  mbuf_buf(mb), mbuf_get_left(mb));
		if (err)
			return err;

		mb->pos = pld_start;
		mb->end = tag_start;

		if (0 != memcmp(tag_calc, tag_pkt, comp->tag_len))
			return EAUTH;

		/*
		 * 3.3.2.  Replay Protection
		 *
		 * Secure replay protection is only possible when
		 * integrity protection is present.
		 */
		if (!srtp_replay_check(&strm->replay_rtp, ix))
			return EALREADY;
	}

	if (comp->aes) {

		union vect128 iv;
		uint8_t *p = mbuf_buf(mb);

		srtp_iv_calc(&iv, &comp->k_s, strm->ssrc, ix);

		aes_set_iv(comp->aes, iv.u8);
		err = aes_decr(comp->aes, p, p, mbuf_get_left(mb));
		if (err)
			return err;
	}

	if (hdr.seq > strm->s_l)
		strm->s_l = hdr.seq;

	mb->pos = start;

	return 0;
}
