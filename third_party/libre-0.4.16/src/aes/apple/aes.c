/**
 * @file apple/aes.c  AES using Apple CommonCrypto API
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_fmt.h>
#include <re_aes.h>
#include <CommonCrypto/CommonCryptor.h>


struct aes {
	CCCryptorRef cryptor;
	uint8_t key[64];
	size_t key_bytes;
};


static void destructor(void *arg)
{
	struct aes *st = arg;

	if (st->cryptor)
		CCCryptorRelease(st->cryptor);
}


int aes_alloc(struct aes **stp, enum aes_mode mode,
	      const uint8_t *key, size_t key_bits,
	      const uint8_t iv[AES_BLOCK_SIZE])
{
	struct aes *st;
	size_t key_bytes = key_bits / 8;
	CCCryptorStatus status;
	int err = 0;

	if (!stp || !key)
		return EINVAL;

	if (mode != AES_MODE_CTR)
		return ENOTSUP;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	if (key_bytes > sizeof(st->key)) {
		err = EINVAL;
		goto out;
	}

	st->key_bytes = key_bytes;
	memcpy(st->key, key, st->key_bytes);

	/* used for both encryption and decryption because CTR is symmetric */
	status = CCCryptorCreateWithMode(kCCEncrypt, kCCModeCTR,
					 kCCAlgorithmAES, ccNoPadding,
					 iv, key, key_bytes, NULL, 0, 0,
					 kCCModeOptionCTR_BE, &st->cryptor);
	if (status != kCCSuccess) {
		err = EPROTO;
		goto out;
	}

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


void aes_set_iv(struct aes *st, const uint8_t iv[AES_BLOCK_SIZE])
{
	CCCryptorStatus status;

	if (!st)
		return;

	/* we must reset the state when updating IV */
	if (st->cryptor) {
		CCCryptorRelease(st->cryptor);
		st->cryptor = NULL;
	}

	status = CCCryptorCreateWithMode(kCCEncrypt, kCCModeCTR,
					 kCCAlgorithmAES, ccNoPadding,
					 iv, st->key, st->key_bytes,
					 NULL, 0, 0, kCCModeOptionCTR_BE,
					 &st->cryptor);
	if (status != kCCSuccess) {
		re_fprintf(stderr, "aes: CCCryptorCreateWithMode error (%d)\n",
			   status);
	}
}


int aes_encr(struct aes *st, uint8_t *out, const uint8_t *in, size_t len)
{
	CCCryptorStatus status;
	size_t moved;

	if (!st || !out || !in)
		return EINVAL;

	status = CCCryptorUpdate(st->cryptor, in, len, out, len, &moved);
	if (status != kCCSuccess) {
		re_fprintf(stderr, "aes: CCCryptorUpdate error (%d)\n",
			   status);
		return EPROTO;
	}

	return 0;
}


int aes_decr(struct aes *st, uint8_t *out, const uint8_t *in, size_t len)
{
	return aes_encr(st, out, in, len);
}
