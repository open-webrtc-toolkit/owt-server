/**
 * @file stun.h  Internal STUN interface
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** STUN Protocol values */
enum {
	STUN_MAGIC_COOKIE = 0x2112a442 /**< Magic Cookie for 3489bis        */
};


/** Calculate STUN message type from method and class */
#define STUN_TYPE(method, class)	    \
	((method)&0x0f80) << 2 |	    \
	((method)&0x0070) << 1 |	    \
	((method)&0x000f) << 0 |	    \
	((class)&0x2)     << 7 |	    \
	((class)&0x1)     << 4


#define STUN_CLASS(type) \
	((type >> 7 | type >> 4) & 0x3)


#define STUN_METHOD(type) \
	((type&0x3e00)>>2 | (type&0x00e0)>>1 | (type&0x000f))


struct stun_hdr {
	uint16_t type;               /**< Message type   */
	uint16_t len;                /**< Payload length */
	uint32_t cookie;             /**< Magic cookie   */
	uint8_t tid[STUN_TID_SIZE];  /**< Transaction ID */
};


struct stun {
	struct list ctl;
	struct stun_conf conf;
	stun_ind_h *indh;
	void *arg;
};

int stun_hdr_encode(struct mbuf *mb, const struct stun_hdr *hdr);
int stun_hdr_decode(struct mbuf *mb, struct stun_hdr *hdr);

int stun_attr_encode(struct mbuf *mb, uint16_t type, const void *v,
		     const uint8_t *tid, uint8_t padding);
int stun_attr_decode(struct stun_attr **attrp, struct mbuf *mb,
		     const uint8_t *tid, struct stun_unknown_attr *ua);
void stun_attr_dump(const struct stun_attr *a);

int  stun_addr_encode(struct mbuf *mb, const struct sa *addr,
		      const uint8_t *tid);
int  stun_addr_decode(struct mbuf *mb, struct sa *addr, const uint8_t *tid);

int stun_ctrans_request(struct stun_ctrans **ctp, struct stun *stun, int proto,
			void *sock, const struct sa *dst, struct mbuf *mb,
			const uint8_t tid[], uint16_t met, const uint8_t *key,
			size_t keylen, stun_resp_h *resph, void *arg);
void stun_ctrans_close(struct stun *stun);
int  stun_ctrans_debug(struct re_printf *pf, const struct stun *stun);
