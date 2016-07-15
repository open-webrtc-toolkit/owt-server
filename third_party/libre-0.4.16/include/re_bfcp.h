/**
 * @file re_bfcp.h Interface to Binary Floor Control Protocol (BFCP)
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** BFCP Versions */
enum {
	BFCP_VER1 = 1,
	BFCP_VER2 = 2,
};

/** BFCP Primitives */
enum bfcp_prim {
	BFCP_FLOOR_REQUEST        =  1,
	BFCP_FLOOR_RELEASE        =  2,
	BFCP_FLOOR_REQUEST_QUERY  =  3,
	BFCP_FLOOR_REQUEST_STATUS =  4,
	BFCP_USER_QUERY           =  5,
	BFCP_USER_STATUS          =  6,
	BFCP_FLOOR_QUERY          =  7,
	BFCP_FLOOR_STATUS         =  8,
	BFCP_CHAIR_ACTION         =  9,
	BFCP_CHAIR_ACTION_ACK     = 10,
	BFCP_HELLO                = 11,
	BFCP_HELLO_ACK            = 12,
	BFCP_ERROR                = 13,
	BFCP_FLOOR_REQ_STATUS_ACK = 14,
	BFCP_FLOOR_STATUS_ACK     = 15,
	BFCP_GOODBYE              = 16,
	BFCP_GOODBYE_ACK          = 17,
};

/** BFCP Attributes */
enum bfcp_attrib {
	BFCP_BENEFICIARY_ID     =  1,
	BFCP_FLOOR_ID           =  2,
	BFCP_FLOOR_REQUEST_ID   =  3,
	BFCP_PRIORITY           =  4,
	BFCP_REQUEST_STATUS     =  5,
	BFCP_ERROR_CODE         =  6,
	BFCP_ERROR_INFO         =  7,
	BFCP_PART_PROV_INFO     =  8,
	BFCP_STATUS_INFO        =  9,
	BFCP_SUPPORTED_ATTRS    = 10,
	BFCP_SUPPORTED_PRIMS    = 11,
	BFCP_USER_DISP_NAME     = 12,
	BFCP_USER_URI           = 13,
	/* grouped: */
	BFCP_BENEFICIARY_INFO   = 14,
	BFCP_FLOOR_REQ_INFO     = 15,
	BFCP_REQUESTED_BY_INFO  = 16,
	BFCP_FLOOR_REQ_STATUS   = 17,
	BFCP_OVERALL_REQ_STATUS = 18,

	/** Mandatory Attribute */
	BFCP_MANDATORY          = 1<<7,
	/** Encode Handler */
	BFCP_ENCODE_HANDLER     = 1<<8,
};

/** BFCP Request Status */
enum bfcp_reqstat {
	BFCP_PENDING   = 1,
	BFCP_ACCEPTED  = 2,
	BFCP_GRANTED   = 3,
	BFCP_DENIED    = 4,
	BFCP_CANCELLED = 5,
	BFCP_RELEASED  = 6,
	BFCP_REVOKED   = 7
};

/** BFCP Error Codes */
enum bfcp_err {
	BFCP_CONF_NOT_EXIST         = 1,
	BFCP_USER_NOT_EXIST         = 2,
	BFCP_UNKNOWN_PRIM           = 3,
	BFCP_UNKNOWN_MAND_ATTR      = 4,
	BFCP_UNAUTH_OPERATION       = 5,
	BFCP_INVALID_FLOOR_ID       = 6,
	BFCP_FLOOR_REQ_ID_NOT_EXIST = 7,
	BFCP_MAX_FLOOR_REQ_REACHED  = 8,
	BFCP_USE_TLS                = 9,
	BFCP_PARSE_ERROR            = 10,
	BFCP_USE_DTLS               = 11,
	BFCP_UNSUPPORTED_VERSION    = 12,
	BFCP_BAD_LENGTH             = 13,
	BFCP_GENERIC_ERROR          = 14,
};

/** BFCP Priority */
enum bfcp_priority {
	BFCP_PRIO_LOWEST  = 0,
	BFCP_PRIO_LOW     = 1,
	BFCP_PRIO_NORMAL  = 2,
	BFCP_PRIO_HIGH    = 3,
	BFCP_PRIO_HIGHEST = 4
};

/** BFCP Transport */
enum bfcp_transp {
	BFCP_UDP,
	BFCP_DTLS,
};

/** BFCP Request Status */
struct bfcp_reqstatus {
	enum bfcp_reqstat status;
	uint8_t qpos;
};

/** BFCP Error Code */
struct bfcp_errcode {
	enum bfcp_err code;
	uint8_t *details;  /* optional */
	size_t len;
};

/** BFCP Supported Attributes */
struct bfcp_supattr {
	enum bfcp_attrib *attrv;
	size_t attrc;
};

/** BFCP Supported Primitives */
struct bfcp_supprim {
	enum bfcp_prim *primv;
	size_t primc;
};

/** BFCP Attribute */
struct bfcp_attr {
	struct le le;
	struct list attrl;
	enum bfcp_attrib type;
	bool mand;
	union bfcp_union {
		/* generic types */
		char *str;
		uint16_t u16;

		/* actual attributes */
		uint16_t beneficiaryid;
		uint16_t floorid;
		uint16_t floorreqid;
		enum bfcp_priority priority;
		struct bfcp_reqstatus reqstatus;
		struct bfcp_errcode errcode;
		char *errinfo;
		char *partprovinfo;
		char *statusinfo;
		struct bfcp_supattr supattr;
		struct bfcp_supprim supprim;
		char *userdname;
		char *useruri;
		uint16_t reqbyid;
	} v;
};

/** BFCP unknown attributes */
struct bfcp_unknown_attr {
	uint8_t typev[16];
	size_t typec;
};

/** BFCP Message */
struct bfcp_msg {
	struct bfcp_unknown_attr uma;
	struct sa src;
	uint8_t ver;
	unsigned r:1;
	unsigned f:1;
	enum bfcp_prim prim;
	uint16_t len;
	uint32_t confid;
	uint16_t tid;
	uint16_t userid;
	struct list attrl;
};

struct tls;
struct bfcp_conn;


/**
 * Defines the BFCP encode handler
 *
 * @param mb  Mbuf to encode into
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
typedef int (bfcp_encode_h)(struct mbuf *mb, void *arg);

/** BFCP Encode */
struct bfcp_encode {
	bfcp_encode_h *ench;
	void *arg;
};


/**
 * Defines the BFCP attribute handler
 *
 * @param attr BFCP attribute
 * @param arg  Handler argument
 *
 * @return True to stop processing, false to continue
 */
typedef bool (bfcp_attr_h)(const struct bfcp_attr *attr, void *arg);


/**
 * Defines the BFCP receive handler
 *
 * @param msg BFCP message
 * @param arg Handler argument
 */
typedef void (bfcp_recv_h)(const struct bfcp_msg *msg, void *arg);


/**
 * Defines the BFCP response handler
 *
 * @param err Error code
 * @param msg BFCP message
 * @param arg Handler argument
 */
typedef void (bfcp_resp_h)(int err, const struct bfcp_msg *msg, void *arg);


/* attr */
int bfcp_attrs_vencode(struct mbuf *mb, unsigned attrc, va_list *ap);
int bfcp_attrs_encode(struct mbuf *mb, unsigned attrc, ...);
struct bfcp_attr *bfcp_attr_subattr(const struct bfcp_attr *attr,
				    enum bfcp_attrib type);
struct bfcp_attr *bfcp_attr_subattr_apply(const struct bfcp_attr *attr,
					  bfcp_attr_h *h, void *arg);
int bfcp_attr_print(struct re_printf *pf, const struct bfcp_attr *attr);
const char *bfcp_attr_name(enum bfcp_attrib type);
const char *bfcp_reqstatus_name(enum bfcp_reqstat status);
const char *bfcp_errcode_name(enum bfcp_err code);


/* msg */
int bfcp_msg_vencode(struct mbuf *mb, uint8_t ver, bool r, enum bfcp_prim prim,
		     uint32_t confid, uint16_t tid, uint16_t userid,
		     unsigned attrc, va_list *ap);
int bfcp_msg_encode(struct mbuf *mb, uint8_t ver, bool r, enum bfcp_prim prim,
		    uint32_t confid, uint16_t tid, uint16_t userid,
		    unsigned attrc, ...);
int bfcp_msg_decode(struct bfcp_msg **msgp, struct mbuf *mb);
struct bfcp_attr *bfcp_msg_attr(const struct bfcp_msg *msg,
				enum bfcp_attrib type);
struct bfcp_attr *bfcp_msg_attr_apply(const struct bfcp_msg *msg,
				      bfcp_attr_h *h, void *arg);
int bfcp_msg_print(struct re_printf *pf, const struct bfcp_msg *msg);
const char *bfcp_prim_name(enum bfcp_prim prim);


/* conn */
int bfcp_listen(struct bfcp_conn **bcp, enum bfcp_transp tp, struct sa *laddr,
		struct tls *tls, bfcp_recv_h *recvh, void *arg);
void *bfcp_sock(const struct bfcp_conn *bc);


/* request */
int bfcp_request(struct bfcp_conn *bc, const struct sa *dst, uint8_t ver,
		 enum bfcp_prim prim, uint32_t confid, uint16_t userid,
		 bfcp_resp_h *resph, void *arg, unsigned attrc, ...);


/* notify */
int bfcp_notify(struct bfcp_conn *bc, const struct sa *dst, uint8_t ver,
		enum bfcp_prim prim, uint32_t confid, uint16_t userid,
		unsigned attrc, ...);


/* reply */
int bfcp_reply(struct bfcp_conn *bc, const struct bfcp_msg *req,
	       enum bfcp_prim prim, unsigned attrc, ...);
int bfcp_edreply(struct bfcp_conn *bc, const struct bfcp_msg *req,
		 enum bfcp_err code, const uint8_t *details, size_t len);
int bfcp_ereply(struct bfcp_conn *bc, const struct bfcp_msg *req,
		enum bfcp_err code);
