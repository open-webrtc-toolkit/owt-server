/**
 * @file hmac/hmac.c  HMAC-SHA1
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_sha.h>
#include <re_hmac.h>


struct hmac {
	uint8_t key[SHA_DIGEST_LENGTH];
	size_t key_len;
};


static void destructor(void *arg)
{
	struct hmac *hmac = arg;

	memset(hmac, 0, sizeof(*hmac));
}


int hmac_create(struct hmac **hmacp, enum hmac_hash hash,
		const uint8_t *key, size_t key_len)
{
	struct hmac *hmac;

	if (!hmacp || !key || !key_len)
		return EINVAL;

	if (hash != HMAC_HASH_SHA1)
		return ENOTSUP;

	if (key_len > SHA_DIGEST_LENGTH)
		return EINVAL;

	hmac = mem_zalloc(sizeof(*hmac), destructor);
	if (!hmac)
		return ENOMEM;

	memcpy(hmac->key, key, key_len);
	hmac->key_len = key_len;

	*hmacp = hmac;

	return 0;
}


int hmac_digest(struct hmac *hmac, uint8_t *md, size_t md_len,
		const uint8_t *data, size_t data_len)
{
	if (!hmac || !md || !md_len || !data || !data_len)
		return EINVAL;

	hmac_sha1(hmac->key, hmac->key_len, data, data_len, md, md_len);

	return 0;
}
