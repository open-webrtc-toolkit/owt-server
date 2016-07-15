/**
 * @file re_srtp.h Secure Real-time Transport Protocol (SRTP)
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum srtp_suite {
	SRTP_AES_CM_128_HMAC_SHA1_32,
	SRTP_AES_CM_128_HMAC_SHA1_80,
	SRTP_AES_256_CM_HMAC_SHA1_32,
	SRTP_AES_256_CM_HMAC_SHA1_80,
};

enum srtp_flags {
	SRTP_UNENCRYPTED_SRTCP = 1<<1,
};

struct srtp;

int srtp_alloc(struct srtp **srtpp, enum srtp_suite suite,
	       const uint8_t *key, size_t key_bytes, int flags);
int srtp_encrypt(struct srtp *srtp, struct mbuf *mb);
int srtp_decrypt(struct srtp *srtp, struct mbuf *mb);
int srtcp_encrypt(struct srtp *srtp, struct mbuf *mb);
int srtcp_decrypt(struct srtp *srtp, struct mbuf *mb);

const char *srtp_suite_name(enum srtp_suite suite);
