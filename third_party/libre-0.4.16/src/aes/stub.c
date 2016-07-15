/**
 * @file aes/stub.c  AES stub
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_aes.h>


int aes_alloc(struct aes **stp, enum aes_mode mode,
	      const uint8_t *key, size_t key_bits,
	      const uint8_t iv[AES_BLOCK_SIZE])
{
	(void)stp;
	(void)mode;
	(void)key;
	(void)key_bits;
	(void)iv;
	return ENOSYS;
}


void aes_set_iv(struct aes *st, const uint8_t iv[AES_BLOCK_SIZE])
{
	(void)st;
	(void)iv;
}


int aes_encr(struct aes *st, uint8_t *out, const uint8_t *in, size_t len)
{
	(void)st;
	(void)out;
	(void)in;
	(void)len;
	return ENOSYS;
}


int aes_decr(struct aes *st, uint8_t *out, const uint8_t *in, size_t len)
{
	(void)st;
	(void)out;
	(void)in;
	(void)len;
	return ENOSYS;
}
