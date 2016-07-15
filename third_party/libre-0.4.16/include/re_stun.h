/**
 * @file re_stun.h  Session Traversal Utilities for (NAT) (STUN)
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** STUN Protocol values */
enum {
	STUN_PORT        = 3478,   /**< STUN Port number */
	STUNS_PORT       = 5349,   /**< STUNS Port number                    */
	STUN_HEADER_SIZE = 20,     /**< Number of bytes in header            */
	STUN_ATTR_HEADER_SIZE = 4, /**< Size of attribute header             */
	STUN_TID_SIZE    = 12,     /**< Number of bytes in transaction ID    */
	STUN_DEFAULT_RTO = 500,    /**< Default Retrans Timeout in [ms]      */
	STUN_DEFAULT_RC  = 7,      /**< Default number of retransmits        */
	STUN_DEFAULT_RM  = 16,     /**< Wait time after last request is sent */
	STUN_DEFAULT_TI  = 39500   /**< Reliable timeout */
};

/** STUN Address Family */
enum stun_af {
	STUN_AF_IPv4 = 0x01,  /**< IPv4 Address Family */
	STUN_AF_IPv6 = 0x02   /**< IPv6 Address Family */
};

/** STUN Transport */
enum stun_transp {
	STUN_TRANSP_UDP = IPPROTO_UDP, /**< UDP-transport (struct udp_sock)  */
	STUN_TRANSP_TCP = IPPROTO_TCP, /**< TCP-transport (struct tcp_conn)  */
	STUN_TRANSP_DTLS,              /**< DTLS-transport (struct tls_conn) */
};

/** STUN Methods */
enum stun_method {
	STUN_METHOD_BINDING    = 0x001,
	STUN_METHOD_ALLOCATE   = 0x003,
	STUN_METHOD_REFRESH    = 0x004,
	STUN_METHOD_SEND       = 0x006,
	STUN_METHOD_DATA       = 0x007,
	STUN_METHOD_CREATEPERM = 0x008,
	STUN_METHOD_CHANBIND   = 0x009,
};

/** STUN Message class */
enum stun_msg_class {
	STUN_CLASS_REQUEST      = 0x0,  /**< STUN Request          */
	STUN_CLASS_INDICATION   = 0x1,  /**< STUN Indication       */
	STUN_CLASS_SUCCESS_RESP = 0x2,  /**< STUN Success Response */
	STUN_CLASS_ERROR_RESP   = 0x3   /**< STUN Error Response   */
};

/** STUN Attributes */
enum stun_attrib {
	/* Comprehension-required range (0x0000-0x7FFF) */
	STUN_ATTR_MAPPED_ADDR        = 0x0001,
	STUN_ATTR_CHANGE_REQ         = 0x0003,
	STUN_ATTR_USERNAME           = 0x0006,
	STUN_ATTR_MSG_INTEGRITY      = 0x0008,
	STUN_ATTR_ERR_CODE           = 0x0009,
	STUN_ATTR_UNKNOWN_ATTR       = 0x000a,
	STUN_ATTR_CHANNEL_NUMBER     = 0x000c,
	STUN_ATTR_LIFETIME           = 0x000d,
	STUN_ATTR_XOR_PEER_ADDR      = 0x0012,
	STUN_ATTR_DATA               = 0x0013,
	STUN_ATTR_REALM              = 0x0014,
	STUN_ATTR_NONCE              = 0x0015,
	STUN_ATTR_XOR_RELAY_ADDR     = 0x0016,
	STUN_ATTR_REQ_ADDR_FAMILY    = 0x0017,
	STUN_ATTR_EVEN_PORT          = 0x0018,
	STUN_ATTR_REQ_TRANSPORT      = 0x0019,
	STUN_ATTR_DONT_FRAGMENT      = 0x001a,
	STUN_ATTR_XOR_MAPPED_ADDR    = 0x0020,
	STUN_ATTR_RSV_TOKEN          = 0x0022,
	STUN_ATTR_PRIORITY           = 0x0024,
	STUN_ATTR_USE_CAND           = 0x0025,
	STUN_ATTR_PADDING            = 0x0026,
	STUN_ATTR_RESP_PORT          = 0x0027,

	/* Comprehension-optional range (0x8000-0xFFFF) */
	STUN_ATTR_SOFTWARE           = 0x8022,
	STUN_ATTR_ALT_SERVER         = 0x8023,
	STUN_ATTR_FINGERPRINT        = 0x8028,
	STUN_ATTR_CONTROLLED         = 0x8029,
	STUN_ATTR_CONTROLLING        = 0x802a,
	STUN_ATTR_RESP_ORIGIN        = 0x802b,
	STUN_ATTR_OTHER_ADDR         = 0x802c,
};


struct stun_change_req {
	bool ip;
	bool port;
};

struct stun_errcode {
	uint16_t code;
	char *reason;
};

struct stun_unknown_attr {
	uint16_t typev[8];
	uint32_t typec;
};

struct stun_even_port {
	bool r;
};

/** Defines a STUN attribute */
struct stun_attr {
	struct le le;
	uint16_t type;
	union {
		/* generic types */
		struct sa sa;
		char *str;
		uint64_t uint64;
		uint32_t uint32;
		uint16_t uint16;
		uint8_t uint8;
		struct mbuf mb;

		/* actual attributes */
		struct sa mapped_addr;
		struct stun_change_req change_req;
		char *username;
		uint8_t msg_integrity[20];
		struct stun_errcode err_code;
		struct stun_unknown_attr unknown_attr;
		uint16_t channel_number;
		uint32_t lifetime;
		struct sa xor_peer_addr;
		struct mbuf data;
		char *realm;
		char *nonce;
		struct sa xor_relay_addr;
		uint8_t req_addr_family;
		struct stun_even_port even_port;
		uint8_t req_transport;
		struct sa xor_mapped_addr;
		uint64_t rsv_token;
		uint32_t priority;
		struct mbuf padding;
		uint16_t resp_port;
		char *software;
		struct sa alt_server;
		uint32_t fingerprint;
		uint64_t controlled;
		uint64_t controlling;
		struct sa resp_origin;
		struct sa other_addr;
	} v;
};


/** STUN Configuration */
struct stun_conf {
	uint32_t rto;  /**< RTO Retransmission TimeOut [ms]        */
	uint32_t rc;   /**< Rc Retransmission count (default 7)    */
	uint32_t rm;   /**< Rm Max retransmissions (default 16)    */
	uint32_t ti;   /**< Ti Timeout for reliable transport [ms] */
	uint8_t tos;   /**< Type-of-service field                  */
};


extern const char *stun_software;
struct stun;
struct stun_msg;
struct stun_ctrans;

typedef void(stun_resp_h)(int err, uint16_t scode, const char *reason,
			  const struct stun_msg *msg, void *arg);
typedef void(stun_ind_h)(struct stun_msg *msg, void *arg);
typedef bool(stun_attr_h)(const struct stun_attr *attr, void *arg);

int  stun_alloc(struct stun **stunp, const struct stun_conf *conf,
		stun_ind_h *indh, void *arg);
struct stun_conf *stun_conf(struct stun *stun);
int  stun_send(int proto, void *sock, const struct sa *dst, struct mbuf *mb);
int  stun_recv(struct stun *stun, struct mbuf *mb);
int  stun_ctrans_recv(struct stun *stun, const struct stun_msg *msg,
		      const struct stun_unknown_attr *ua);
struct re_printf;
int  stun_debug(struct re_printf *pf, const struct stun *stun);

int  stun_request(struct stun_ctrans **ctp, struct stun *stun, int proto,
		  void *sock, const struct sa *dst, size_t presz,
		  uint16_t method, const uint8_t *key, size_t keylen, bool fp,
		  stun_resp_h *resph, void *arg, uint32_t attrc, ...);
int  stun_reply(int proto, void *sock, const struct sa *dst, size_t presz,
		const struct stun_msg *req, const uint8_t *key,
		size_t keylen, bool fp, uint32_t attrc, ...);
int  stun_ereply(int proto, void *sock, const struct sa *dst, size_t presz,
		 const struct stun_msg *req, uint16_t scode,
		 const char *reason, const uint8_t *key, size_t keylen,
		 bool fp, uint32_t attrc, ...);
int  stun_indication(int proto, void *sock, const struct sa *dst, size_t presz,
		     uint16_t method, const uint8_t *key, size_t keylen,
		     bool fp, uint32_t attrc, ...);

int  stun_msg_vencode(struct mbuf *mb, uint16_t method, uint8_t cls,
		      const uint8_t *tid, const struct stun_errcode *ec,
		      const uint8_t *key, size_t keylen, bool fp,
		      uint8_t padding, uint32_t attrc, va_list ap);
int  stun_msg_encode(struct mbuf *mb, uint16_t method, uint8_t cls,
		     const uint8_t *tid, const struct stun_errcode *ec,
		     const uint8_t *key, size_t keylen, bool fp,
		     uint8_t padding, uint32_t attrc, ...);
int  stun_msg_decode(struct stun_msg **msgpp, struct mbuf *mb,
		     struct stun_unknown_attr *ua);
uint16_t stun_msg_type(const struct stun_msg *msg);
uint16_t stun_msg_class(const struct stun_msg *msg);
uint16_t stun_msg_method(const struct stun_msg *msg);
bool stun_msg_mcookie(const struct stun_msg *msg);
const uint8_t *stun_msg_tid(const struct stun_msg *msg);
struct stun_attr *stun_msg_attr(const struct stun_msg *msg, uint16_t type);
struct stun_attr *stun_msg_attr_apply(const struct stun_msg *msg,
				      stun_attr_h *h, void *arg);
int  stun_msg_chk_mi(const struct stun_msg *msg, const uint8_t *key,
		     size_t keylen);
int  stun_msg_chk_fingerprint(const struct stun_msg *msg);
void stun_msg_dump(const struct stun_msg *msg);

const char *stun_class_name(uint16_t cls);
const char *stun_method_name(uint16_t method);
const char *stun_attr_name(uint16_t type);
const char *stun_transp_name(enum stun_transp tp);


/* DNS Discovery of a STUN Server */
extern const char *stun_proto_udp;
extern const char *stun_proto_tcp;

extern const char *stun_usage_binding;
extern const char *stuns_usage_binding;
extern const char *stun_usage_relay;
extern const char *stuns_usage_relay;
extern const char *stun_usage_behavior;
extern const char *stuns_usage_behavior;


/**
 * Defines the STUN Server Discovery handler
 *
 * @param err Errorcode
 * @param srv IP Address and port of STUN Server
 * @param arg Handler argument
 */
typedef void (stun_dns_h)(int err, const struct sa *srv, void *arg);

struct stun_dns;
struct dnsc;
int  stun_server_discover(struct stun_dns **dnsp, struct dnsc *dnsc,
			  const char *service, const char *proto,
			  int af, const char *domain, uint16_t port,
			  stun_dns_h *dnsh, void *arg);


/* NAT Keepalives */
struct stun_keepalive;

/**
 * Defines the STUN Keepalive Mapped-Address handler
 *
 * @param err Errorcode
 * @param map Mapped Address
 * @param arg Handler argument
 */
typedef void (stun_mapped_addr_h)(int err, const struct sa *map, void *arg);


int  stun_keepalive_alloc(struct stun_keepalive **skap,
			  int proto, void *sock, int layer,
			  const struct sa *dst, const struct stun_conf *conf,
			  stun_mapped_addr_h *mah, void *arg);
void stun_keepalive_enable(struct stun_keepalive *ska, uint32_t interval);


/* STUN Reason Phrase */
extern const char *stun_reason_300;
extern const char *stun_reason_400;
extern const char *stun_reason_401;
extern const char *stun_reason_403;
extern const char *stun_reason_420;
extern const char *stun_reason_437;
extern const char *stun_reason_438;
extern const char *stun_reason_440;
extern const char *stun_reason_441;
extern const char *stun_reason_442;
extern const char *stun_reason_443;
extern const char *stun_reason_486;
extern const char *stun_reason_500;
extern const char *stun_reason_508;
