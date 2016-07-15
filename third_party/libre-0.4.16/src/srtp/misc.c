/**
 * @file srtp/misc.c  SRTP functions
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_aes.h>
#include <re_sa.h>
#include <re_srtp.h>
#include "srtp.h"


/*
 * Appendix A: Pseudocode for Index Determination
 *
 * In the following, signed arithmetic is assumed.
 */
uint64_t srtp_get_index(uint32_t roc, uint16_t s_l, uint16_t seq)
{
	int v;

	if (s_l < 32768) {

		if ((int)seq - (int)s_l > 32768)
			v = (roc-1) & 0xffffffffu;
		else
			v = roc;
	}
	else {
		if ((int)s_l - 32768 > seq)
			v = (roc+1) & 0xffffffffu;
		else
			v = roc;
	}

	return seq + v*65536;
}


int srtp_derive(uint8_t *out, size_t out_len, uint8_t label,
		const uint8_t *master_key, size_t key_bytes,
		const uint8_t *master_salt, size_t salt_bytes)
{
	uint8_t x[AES_BLOCK_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	static const uint8_t null[AES_BLOCK_SIZE * 2];
	struct aes *aes;
	int err;

	if (!out || !master_key || !master_salt)
		return EINVAL;

	if (out_len > sizeof(null) || salt_bytes > sizeof(x))
		return EINVAL;

	memcpy(x, master_salt, salt_bytes);
	x[7] ^= label;

	err = aes_alloc(&aes, AES_MODE_CTR, master_key, key_bytes*8, x);
	if (err)
		return err;

	err = aes_encr(aes, out, null, out_len);

	mem_deref(aes);

	return err;

}


void srtp_iv_calc(union vect128 *iv, const union vect128 *k_s,
		  uint32_t ssrc, uint64_t ix)
{
	if (!iv || !k_s)
		return;

	iv->u32[0] = k_s->u32[0];
	iv->u32[1] = k_s->u32[1] ^ htonl(ssrc);
	iv->u32[2] = k_s->u32[2] ^ htonl((uint32_t)(ix>>16));
	iv->u16[6] = k_s->u16[6] ^ htons((uint16_t)ix);
	iv->u16[7] = 0;
}


const char *srtp_suite_name(enum srtp_suite suite)
{
	switch (suite) {

	case SRTP_AES_CM_128_HMAC_SHA1_32:  return "AES_CM_128_HMAC_SHA1_32";
	case SRTP_AES_CM_128_HMAC_SHA1_80:  return "AES_CM_128_HMAC_SHA1_80";
	case SRTP_AES_256_CM_HMAC_SHA1_32:  return "AES_256_CM_HMAC_SHA1_32";
	case SRTP_AES_256_CM_HMAC_SHA1_80:  return "AES_256_CM_HMAC_SHA1_80";
	default:                            return "?";
	}
}
