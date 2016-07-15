/**
 * @file srtp.h  Secure Real-time Transport Protocol (SRTP) -- internal
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	SRTP_SALT_SIZE = 14
};


/** Defines a 128-bit vector in network order */
union vect128 {
	uint64_t u64[ 2];
	uint32_t u32[ 4];
	uint16_t u16[ 8];
	uint8_t   u8[16];
};

/** Replay protection */
struct replay {
	uint64_t bitmap;   /**< Session state - must be 64 bits */
	uint64_t lix;      /**< Last received index             */
};

/** SRTP stream/context -- shared state between RTP/RTCP */
struct srtp_stream {
	struct le le;              /**< Linked-list element                */
	struct replay replay_rtp;  /**< recv -- replay protection for RTP  */
	struct replay replay_rtcp; /**< recv -- replay protection for RTCP */
	uint32_t ssrc;             /**< SSRC -- lookup key                 */
	uint32_t roc;              /**< send/recv Roll-Over Counter (ROC)  */
	uint16_t s_l;              /**< send/recv -- highest SEQ number    */
	bool s_l_set;              /**< True if s_l has been set           */
	uint32_t rtcp_index;       /**< RTCP-index for sending (31-bits)   */
};

/** SRTP Session */
struct srtp {
	struct comp {
		struct aes *aes;    /**< AES Context                       */
		struct hmac *hmac;  /**< HMAC Context                      */
		union vect128 k_s;  /**< Derived salting key (14 bytes)    */
		size_t tag_len;     /**< Authentication tag length [bytes] */
	} rtp, rtcp;

	struct list streaml;        /**< SRTP-streams (struct srtp_stream) */
};


int stream_get(struct srtp_stream **strmp, struct srtp *srtp, uint32_t ssrc);
int stream_get_seq(struct srtp_stream **strmp, struct srtp *srtp,
		   uint32_t ssrc, uint16_t seq);


int  srtp_derive(uint8_t *out, size_t out_len, uint8_t label,
		 const uint8_t *master_key, size_t key_bytes,
		 const uint8_t *master_salt, size_t salt_bytes);
void srtp_iv_calc(union vect128 *iv, const union vect128 *k_s,
		  uint32_t ssrc, uint64_t ix);
uint64_t srtp_get_index(uint32_t roc, uint16_t s_l, uint16_t seq);


/* Replay protection */

void srtp_replay_init(struct replay *replay);
bool srtp_replay_check(struct replay *replay, uint64_t ix);
