/**
 * @file openssl/hmac.c  HMAC using OpenSSL
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <openssl/hmac.h>
#include <openssl/err.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_hmac.h>


struct hmac {
	HMAC_CTX ctx;
};


static void destructor(void *arg)
{
	struct hmac *hmac = arg;

	HMAC_CTX_cleanup(&hmac->ctx);
}


int hmac_create(struct hmac **hmacp, enum hmac_hash hash,
		const uint8_t *key, size_t key_len)
{
	struct hmac *hmac;
	const EVP_MD *evp;
	int err = 0;

	if (!hmacp || !key || !key_len)
		return EINVAL;

	switch (hash) {

	case HMAC_HASH_SHA1:
		evp = EVP_sha1();
		break;

	case HMAC_HASH_SHA256:
		evp = EVP_sha256();
		break;

	default:
		return ENOTSUP;
	}

	hmac = mem_zalloc(sizeof(*hmac), destructor);
	if (!hmac)
		return ENOMEM;

	HMAC_CTX_init(&hmac->ctx);

#if (OPENSSL_VERSION_NUMBER >= 0x00909000)
	if (!HMAC_Init_ex(&hmac->ctx, key, (int)key_len, evp, NULL)) {
		ERR_clear_error();
		err = EPROTO;
	}
#else
	HMAC_Init_ex(&hmac->ctx, key, (int)key_len, evp, NULL);
#endif

	if (err)
		mem_deref(hmac);
	else
		*hmacp = hmac;

	return err;
}


int hmac_digest(struct hmac *hmac, uint8_t *md, size_t md_len,
		const uint8_t *data, size_t data_len)
{
	unsigned int len = (unsigned int)md_len;

	if (!hmac || !md || !md_len || !data || !data_len)
		return EINVAL;

#if (OPENSSL_VERSION_NUMBER >= 0x00909000)
	/* the HMAC context must be reset here */
	if (!HMAC_Init_ex(&hmac->ctx, 0, 0, 0, NULL))
		goto error;

	if (!HMAC_Update(&hmac->ctx, data, (int)data_len))
		goto error;
	if (!HMAC_Final(&hmac->ctx, md, &len))
		goto error;

	return 0;

 error:
	ERR_clear_error();
	return EPROTO;

#else
	/* the HMAC context must be reset here */
	HMAC_Init_ex(&hmac->ctx, 0, 0, 0, NULL);

	HMAC_Update(&hmac->ctx, data, (int)data_len);
	HMAC_Final(&hmac->ctx, md, &len);

	return 0;
#endif
}
