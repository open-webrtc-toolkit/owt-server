/**
 * @file re_sip.h  Session Initiation Protocol
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	SIP_PORT     = 5060,
	SIP_PORT_TLS = 5061,
};

/** SIP Transport */
enum sip_transp {
	SIP_TRANSP_NONE = -1,
	SIP_TRANSP_UDP = 0,
	SIP_TRANSP_TCP,
	SIP_TRANSP_TLS,
	SIP_TRANSPC,
};


/** SIP Header ID (perfect hash value) */
enum sip_hdrid {
	SIP_HDR_ACCEPT                        = 3186,
	SIP_HDR_ACCEPT_CONTACT                =  232,
	SIP_HDR_ACCEPT_ENCODING               =  708,
	SIP_HDR_ACCEPT_LANGUAGE               = 2867,
	SIP_HDR_ACCEPT_RESOURCE_PRIORITY      = 1848,
	SIP_HDR_ALERT_INFO                    =  274,
	SIP_HDR_ALLOW                         = 2429,
	SIP_HDR_ALLOW_EVENTS                  =   66,
	SIP_HDR_ANSWER_MODE                   = 2905,
	SIP_HDR_AUTHENTICATION_INFO           = 3144,
	SIP_HDR_AUTHORIZATION                 = 2503,
	SIP_HDR_CALL_ID                       = 3095,
	SIP_HDR_CALL_INFO                     =  586,
	SIP_HDR_CONTACT                       =  229,
	SIP_HDR_CONTENT_DISPOSITION           = 1425,
	SIP_HDR_CONTENT_ENCODING              =  580,
	SIP_HDR_CONTENT_LANGUAGE              = 3371,
	SIP_HDR_CONTENT_LENGTH                = 3861,
	SIP_HDR_CONTENT_TYPE                  =  809,
	SIP_HDR_CSEQ                          =  746,
	SIP_HDR_DATE                          = 1027,
	SIP_HDR_ENCRYPTION                    = 3125,
	SIP_HDR_ERROR_INFO                    =   21,
	SIP_HDR_EVENT                         = 3286,
	SIP_HDR_EXPIRES                       = 1983,
	SIP_HDR_FLOW_TIMER                    =  584,
	SIP_HDR_FROM                          = 1963,
	SIP_HDR_HIDE                          =  283,
	SIP_HDR_HISTORY_INFO                  = 2582,
	SIP_HDR_IDENTITY                      = 2362,
	SIP_HDR_IDENTITY_INFO                 =  980,
	SIP_HDR_IN_REPLY_TO                   = 1577,
	SIP_HDR_JOIN                          = 3479,
	SIP_HDR_MAX_BREADTH                   = 3701,
	SIP_HDR_MAX_FORWARDS                  = 3549,
	SIP_HDR_MIME_VERSION                  = 3659,
	SIP_HDR_MIN_EXPIRES                   = 1121,
	SIP_HDR_MIN_SE                        = 2847,
	SIP_HDR_ORGANIZATION                  = 3247,
	SIP_HDR_P_ACCESS_NETWORK_INFO         = 1662,
	SIP_HDR_P_ANSWER_STATE                =   42,
	SIP_HDR_P_ASSERTED_IDENTITY           = 1233,
	SIP_HDR_P_ASSOCIATED_URI              =  900,
	SIP_HDR_P_CALLED_PARTY_ID             = 3347,
	SIP_HDR_P_CHARGING_FUNCTION_ADDRESSES = 2171,
	SIP_HDR_P_CHARGING_VECTOR             =   25,
	SIP_HDR_P_DCS_TRACE_PARTY_ID          = 3027,
	SIP_HDR_P_DCS_OSPS                    = 1788,
	SIP_HDR_P_DCS_BILLING_INFO            = 2017,
	SIP_HDR_P_DCS_LAES                    =  693,
	SIP_HDR_P_DCS_REDIRECT                = 1872,
	SIP_HDR_P_EARLY_MEDIA                 = 2622,
	SIP_HDR_P_MEDIA_AUTHORIZATION         = 1035,
	SIP_HDR_P_PREFERRED_IDENTITY          = 1263,
	SIP_HDR_P_PROFILE_KEY                 = 1904,
	SIP_HDR_P_REFUSED_URI_LIST            = 1047,
	SIP_HDR_P_SERVED_USER                 = 1588,
	SIP_HDR_P_USER_DATABASE               = 2827,
	SIP_HDR_P_VISITED_NETWORK_ID          = 3867,
	SIP_HDR_PATH                          = 2741,
	SIP_HDR_PERMISSION_MISSING            = 1409,
	SIP_HDR_PRIORITY                      = 3520,
	SIP_HDR_PRIV_ANSWER_MODE              = 2476,
	SIP_HDR_PRIVACY                       = 3150,
	SIP_HDR_PROXY_AUTHENTICATE            =  116,
	SIP_HDR_PROXY_AUTHORIZATION           = 2363,
	SIP_HDR_PROXY_REQUIRE                 = 3562,
	SIP_HDR_RACK                          = 2523,
	SIP_HDR_REASON                        = 3732,
	SIP_HDR_RECORD_ROUTE                  =  278,
	SIP_HDR_REFER_SUB                     = 2458,
	SIP_HDR_REFER_TO                      = 1521,
	SIP_HDR_REFERRED_BY                   = 3456,
	SIP_HDR_REJECT_CONTACT                =  285,
	SIP_HDR_REPLACES                      = 2534,
	SIP_HDR_REPLY_TO                      = 2404,
	SIP_HDR_REQUEST_DISPOSITION           = 3715,
	SIP_HDR_REQUIRE                       = 3905,
	SIP_HDR_RESOURCE_PRIORITY             = 1643,
	SIP_HDR_RESPONSE_KEY                  = 1548,
	SIP_HDR_RETRY_AFTER                   =  409,
	SIP_HDR_ROUTE                         =  661,
	SIP_HDR_RSEQ                          =  445,
	SIP_HDR_SECURITY_CLIENT               = 1358,
	SIP_HDR_SECURITY_SERVER               =  811,
	SIP_HDR_SECURITY_VERIFY               =  519,
	SIP_HDR_SERVER                        =  973,
	SIP_HDR_SERVICE_ROUTE                 = 1655,
	SIP_HDR_SESSION_EXPIRES               = 1979,
	SIP_HDR_SIP_ETAG                      = 1997,
	SIP_HDR_SIP_IF_MATCH                  = 3056,
	SIP_HDR_SUBJECT                       = 1043,
	SIP_HDR_SUBSCRIPTION_STATE            = 2884,
	SIP_HDR_SUPPORTED                     =  119,
	SIP_HDR_TARGET_DIALOG                 = 3450,
	SIP_HDR_TIMESTAMP                     =  938,
	SIP_HDR_TO                            = 1449,
	SIP_HDR_TRIGGER_CONSENT               = 3180,
	SIP_HDR_UNSUPPORTED                   =  982,
	SIP_HDR_USER_AGENT                    = 4064,
	SIP_HDR_VIA                           = 3961,
	SIP_HDR_WARNING                       = 2108,
	SIP_HDR_WWW_AUTHENTICATE              = 2763,

	SIP_HDR_NONE = -1
};


enum {
	SIP_T1 =  500,
	SIP_T2 = 4000,
	SIP_T4 = 5000,
};


/** SIP Via header */
struct sip_via {
	struct pl sentby;
	struct sa addr;
	struct pl params;
	struct pl branch;
	struct pl val;
	enum sip_transp tp;
};

/** SIP Address */
struct sip_addr {
	struct pl dname;
	struct pl auri;
	struct uri uri;
	struct pl params;
};

/** SIP Tag address */
struct sip_taddr {
	struct pl dname;
	struct pl auri;
	struct uri uri;
	struct pl params;
	struct pl tag;
	struct pl val;
};

/** SIP CSeq header */
struct sip_cseq {
	struct pl met;
	uint32_t num;
};

/** SIP Header */
struct sip_hdr {
	struct le le;          /**< Linked-list element    */
	struct le he;          /**< Hash-table element     */
	struct pl name;        /**< SIP Header name        */
	struct pl val;         /**< SIP Header value       */
	enum sip_hdrid id;     /**< SIP Header id (unique) */
};

/** SIP Message */
struct sip_msg {
	struct sa src;         /**< Source network address               */
	struct sa dst;         /**< Destination network address          */
	struct pl ver;         /**< SIP Version number                   */
	struct pl met;         /**< Request method                       */
	struct pl ruri;        /**< Raw request URI                      */
	struct uri uri;        /**< Parsed request URI                   */
	uint16_t scode;        /**< Response status code                 */
	struct pl reason;      /**< Response reason phrase               */
	struct list hdrl;      /**< List of SIP Headers (struct sip_hdr) */
	struct sip_via via;    /**< Parsed first Via header              */
	struct sip_taddr to;   /**< Parsed To header                     */
	struct sip_taddr from; /**< Parsed From header                   */
	struct sip_cseq cseq;  /**< Parsed CSeq header                   */
	struct msg_ctype ctyp; /**< Content Type                         */
	struct pl callid;      /**< Cached Call-ID header                */
	struct pl maxfwd;      /**< Cached Max-Forwards header           */
	struct pl expires;     /**< Cached Expires header                */
	struct pl clen;        /**< Cached Content-Length header         */
	struct hash *hdrht;    /**< Hash-table with all SIP headers      */
	struct mbuf *mb;       /**< Buffer containing the SIP message    */
	void *sock;            /**< Transport socket                     */
	uint64_t tag;          /**< Opaque tag                           */
	enum sip_transp tp;    /**< SIP Transport                        */
	bool req;              /**< True if Request, False if Response  */
};

/** SIP Loop-state */
struct sip_loopstate {
	uint32_t failc;
	uint16_t last_scode;
};

/** SIP Contact */
struct sip_contact {
	const char *uri;
	const struct sa *addr;
	enum sip_transp tp;
};

struct sip;
struct sip_lsnr;
struct sip_request;
struct sip_strans;
struct sip_auth;
struct sip_dialog;
struct sip_keepalive;
struct dnsc;

typedef bool(sip_msg_h)(const struct sip_msg *msg, void *arg);
typedef int(sip_send_h)(enum sip_transp tp, const struct sa *src,
			const struct sa *dst, struct mbuf *mb, void *arg);
typedef void(sip_resp_h)(int err, const struct sip_msg *msg, void *arg);
typedef void(sip_cancel_h)(void *arg);
typedef void(sip_exit_h)(void *arg);
typedef int(sip_auth_h)(char **username, char **password, const char *realm,
			void *arg);
typedef bool(sip_hdr_h)(const struct sip_hdr *hdr, const struct sip_msg *msg,
			void *arg);
typedef void(sip_keepalive_h)(int err, void *arg);


/* sip */
int  sip_alloc(struct sip **sipp, struct dnsc *dnsc, uint32_t ctsz,
	       uint32_t stsz, uint32_t tcsz, const char *software,
	       sip_exit_h *exith, void *arg);
void sip_close(struct sip *sip, bool force);
int  sip_listen(struct sip_lsnr **lsnrp, struct sip *sip, bool req,
		sip_msg_h *msgh, void *arg);
int  sip_debug(struct re_printf *pf, const struct sip *sip);
int  sip_send(struct sip *sip, void *sock, enum sip_transp tp,
	      const struct sa *dst, struct mbuf *mb);


/* transport */
int  sip_transp_add(struct sip *sip, enum sip_transp tp,
		    const struct sa *laddr, ...);
void sip_transp_flush(struct sip *sip);
bool sip_transp_isladdr(const struct sip *sip, enum sip_transp tp,
			const struct sa *laddr);
const char *sip_transp_name(enum sip_transp tp);
const char *sip_transp_param(enum sip_transp tp);
uint16_t sip_transp_port(enum sip_transp tp, uint16_t port);
int  sip_transp_laddr(struct sip *sip, struct sa *laddr, enum sip_transp tp,
		      const struct sa *dst);


/* request */
int sip_request(struct sip_request **reqp, struct sip *sip, bool stateful,
		const char *met, int metl, const char *uri, int uril,
		const struct uri *route, struct mbuf *mb,
		sip_send_h *sendh, sip_resp_h *resph, void *arg);
int sip_requestf(struct sip_request **reqp, struct sip *sip, bool stateful,
		 const char *met, const char *uri, const struct uri *route,
		 struct sip_auth *auth, sip_send_h *sendh, sip_resp_h *resph,
		 void *arg, const char *fmt, ...);
int sip_drequestf(struct sip_request **reqp, struct sip *sip, bool stateful,
		  const char *met, struct sip_dialog *dlg, uint32_t cseq,
		  struct sip_auth *auth, sip_send_h *sendh, sip_resp_h *resph,
		  void *arg, const char *fmt, ...);
void sip_request_cancel(struct sip_request *req);
bool sip_request_loops(struct sip_loopstate *ls, uint16_t scode);
void sip_loopstate_reset(struct sip_loopstate *ls);


/* reply */
int  sip_strans_alloc(struct sip_strans **stp, struct sip *sip,
		      const struct sip_msg *msg, sip_cancel_h *cancelh,
		      void *arg);
int  sip_strans_reply(struct sip_strans **stp, struct sip *sip,
		      const struct sip_msg *msg, const struct sa *dst,
		      uint16_t scode, struct mbuf *mb);
int  sip_treplyf(struct sip_strans **stp, struct mbuf **mbp, struct sip *sip,
		 const struct sip_msg *msg, bool rec_route, uint16_t scode,
		 const char *reason, const char *fmt, ...);
int  sip_treply(struct sip_strans **stp, struct sip *sip,
		const struct sip_msg *msg, uint16_t scode, const char *reason);
int  sip_replyf(struct sip *sip, const struct sip_msg *msg, uint16_t scode,
		const char *reason, const char *fmt, ...);
int  sip_reply(struct sip *sip, const struct sip_msg *msg, uint16_t scode,
	       const char *reason);
void sip_reply_addr(struct sa *addr, const struct sip_msg *msg, bool rport);


/* auth */
int  sip_auth_authenticate(struct sip_auth *auth, const struct sip_msg *msg);
int  sip_auth_alloc(struct sip_auth **authp, sip_auth_h *authh,
		    void *arg, bool ref);
void sip_auth_reset(struct sip_auth *auth);


/* contact */
void sip_contact_set(struct sip_contact *contact, const char *uri,
		     const struct sa *addr, enum sip_transp tp);
int  sip_contact_print(struct re_printf *pf,
		       const struct sip_contact *contact);


/* dialog */
int  sip_dialog_alloc(struct sip_dialog **dlgp,
		      const char *uri, const char *to_uri,
		      const char *from_name, const char *from_uri,
		      const char *routev[], uint32_t routec);
int  sip_dialog_accept(struct sip_dialog **dlgp, const struct sip_msg *msg);
int  sip_dialog_create(struct sip_dialog *dlg, const struct sip_msg *msg);
int  sip_dialog_fork(struct sip_dialog **dlgp, struct sip_dialog *odlg,
		     const struct sip_msg *msg);
int  sip_dialog_update(struct sip_dialog *dlg, const struct sip_msg *msg);
bool sip_dialog_rseq_valid(struct sip_dialog *dlg, const struct sip_msg *msg);
const char *sip_dialog_callid(const struct sip_dialog *dlg);
uint32_t sip_dialog_lseq(const struct sip_dialog *dlg);
bool sip_dialog_established(const struct sip_dialog *dlg);
bool sip_dialog_cmp(const struct sip_dialog *dlg, const struct sip_msg *msg);
bool sip_dialog_cmp_half(const struct sip_dialog *dlg,
			 const struct sip_msg *msg);


/* msg */
int sip_msg_decode(struct sip_msg **msgp, struct mbuf *mb);
const struct sip_hdr *sip_msg_hdr(const struct sip_msg *msg,
				  enum sip_hdrid id);
const struct sip_hdr *sip_msg_hdr_apply(const struct sip_msg *msg,
					bool fwd, enum sip_hdrid id,
					sip_hdr_h *h, void *arg);
const struct sip_hdr *sip_msg_xhdr(const struct sip_msg *msg,
				   const char *name);
const struct sip_hdr *sip_msg_xhdr_apply(const struct sip_msg *msg,
					 bool fwd, const char *name,
					 sip_hdr_h *h, void *arg);
uint32_t sip_msg_hdr_count(const struct sip_msg *msg, enum sip_hdrid id);
uint32_t sip_msg_xhdr_count(const struct sip_msg *msg, const char *name);
bool sip_msg_hdr_has_value(const struct sip_msg *msg, enum sip_hdrid id,
			   const char *value);
bool sip_msg_xhdr_has_value(const struct sip_msg *msg, const char *name,
			    const char *value);
struct tcp_conn *sip_msg_tcpconn(const struct sip_msg *msg);
void sip_msg_dump(const struct sip_msg *msg);

int sip_addr_decode(struct sip_addr *addr, const struct pl *pl);
int sip_via_decode(struct sip_via *via, const struct pl *pl);
int sip_cseq_decode(struct sip_cseq *cseq, const struct pl *pl);


/* keepalive */
int sip_keepalive_start(struct sip_keepalive **kap, struct sip *sip,
			const struct sip_msg *msg, uint32_t interval,
			sip_keepalive_h *kah, void *arg);
