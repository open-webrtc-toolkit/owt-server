/**
 * @file re_hmac.h  Interface to HMAC functions
 *
 * Copyright (C) 2010 Creytiv.com
 */


void hmac_sha1(const uint8_t *k,   /* secret key */
	       size_t         lk,  /* length of the key in bytes */
	       const uint8_t *d,   /* data */
	       size_t         ld,  /* length of data in bytes */
	       uint8_t*       out, /* output buffer, at least "t" bytes */
	       size_t         t);


enum hmac_hash {
	HMAC_HASH_SHA1,
	HMAC_HASH_SHA256
};

struct hmac;

int  hmac_create(struct hmac **hmacp, enum hmac_hash hash,
		 const uint8_t *key, size_t key_len);
int  hmac_digest(struct hmac *hmac, uint8_t *md, size_t md_len,
		 const uint8_t *data, size_t data_len);
