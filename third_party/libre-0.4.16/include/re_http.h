/**
 * @file re_http.h  Hypertext Transfer Protocol
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** HTTP Header ID (perfect hash value) */
enum http_hdrid {
	HTTP_HDR_ACCEPT                   = 3186,
	HTTP_HDR_ACCEPT_CHARSET           =   24,
	HTTP_HDR_ACCEPT_ENCODING          =  708,
	HTTP_HDR_ACCEPT_LANGUAGE          = 2867,
	HTTP_HDR_ACCEPT_RANGES            = 3027,
	HTTP_HDR_AGE                      =  742,
	HTTP_HDR_ALLOW                    = 2429,
	HTTP_HDR_AUTHORIZATION            = 2503,
	HTTP_HDR_CACHE_CONTROL            = 2530,
	HTTP_HDR_CONNECTION               =  865,
	HTTP_HDR_CONTENT_ENCODING         =  580,
	HTTP_HDR_CONTENT_LANGUAGE         = 3371,
	HTTP_HDR_CONTENT_LENGTH           = 3861,
	HTTP_HDR_CONTENT_LOCATION         = 3927,
	HTTP_HDR_CONTENT_MD5              =  406,
	HTTP_HDR_CONTENT_RANGE            = 2846,
	HTTP_HDR_CONTENT_TYPE             =  809,
	HTTP_HDR_DATE                     = 1027,
	HTTP_HDR_ETAG                     = 2392,
	HTTP_HDR_EXPECT                   = 1550,
	HTTP_HDR_EXPIRES                  = 1983,
	HTTP_HDR_FROM                     = 1963,
	HTTP_HDR_HOST                     = 3191,
	HTTP_HDR_IF_MATCH                 = 2684,
	HTTP_HDR_IF_MODIFIED_SINCE        = 2187,
	HTTP_HDR_IF_NONE_MATCH            = 4030,
	HTTP_HDR_IF_RANGE                 = 2220,
	HTTP_HDR_IF_UNMODIFIED_SINCE      =  962,
	HTTP_HDR_LAST_MODIFIED            = 2946,
	HTTP_HDR_LOCATION                 = 2514,
	HTTP_HDR_MAX_FORWARDS             = 3549,
	HTTP_HDR_PRAGMA                   = 1673,
	HTTP_HDR_PROXY_AUTHENTICATE       =  116,
	HTTP_HDR_PROXY_AUTHORIZATION      = 2363,
	HTTP_HDR_RANGE                    = 4004,
	HTTP_HDR_REFERER                  = 2991,
	HTTP_HDR_RETRY_AFTER              =  409,
	HTTP_HDR_SEC_WEBSOCKET_ACCEPT     = 2959,
	HTTP_HDR_SEC_WEBSOCKET_EXTENSIONS = 2937,
	HTTP_HDR_SEC_WEBSOCKET_KEY        =  746,
	HTTP_HDR_SEC_WEBSOCKET_PROTOCOL   = 2076,
	HTTP_HDR_SEC_WEBSOCKET_VERSION    = 3158,
	HTTP_HDR_SERVER                   =  973,
	HTTP_HDR_TE                       = 2035,
	HTTP_HDR_TRAILER                  = 2577,
	HTTP_HDR_TRANSFER_ENCODING        = 2115,
	HTTP_HDR_UPGRADE                  =  717,
	HTTP_HDR_USER_AGENT               = 4064,
	HTTP_HDR_VARY                     = 3076,
	HTTP_HDR_VIA                      = 3961,
	HTTP_HDR_WARNING                  = 2108,
	HTTP_HDR_WWW_AUTHENTICATE         = 2763,

	HTTP_HDR_NONE = -1
};


/** HTTP Header */
struct http_hdr {
	struct le le;          /**< Linked-list element     */
	struct pl name;        /**< HTTP Header name        */
	struct pl val;         /**< HTTP Header value       */
	enum http_hdrid id;    /**< HTTP Header id (unique) */
};

/** HTTP Message */
struct http_msg {
	struct pl ver;         /**< HTTP Version number                    */
	struct pl met;         /**< Request Method                         */
	struct pl path;        /**< Request path/resource                  */
	struct pl prm;         /**< Request parameters                     */
	uint16_t scode;        /**< Response Status code                   */
	struct pl reason;      /**< Response Reason phrase                 */
	struct list hdrl;      /**< List of HTTP headers (struct http_hdr) */
	struct msg_ctype ctyp; /**< Content-type                           */
	struct mbuf *mb;       /**< Buffer containing the HTTP message     */
	uint32_t clen;         /**< Content length                         */
};

typedef bool(http_hdr_h)(const struct http_hdr *hdr, void *arg);

int  http_msg_decode(struct http_msg **msgp, struct mbuf *mb, bool req);


const struct http_hdr *http_msg_hdr(const struct http_msg *msg,
				    enum http_hdrid id);
const struct http_hdr *http_msg_hdr_apply(const struct http_msg *msg,
					  bool fwd, enum http_hdrid id,
					  http_hdr_h *h, void *arg);
const struct http_hdr *http_msg_xhdr(const struct http_msg *msg,
				     const char *name);
const struct http_hdr *http_msg_xhdr_apply(const struct http_msg *msg,
					   bool fwd, const char *name,
					   http_hdr_h *h, void *arg);
uint32_t http_msg_hdr_count(const struct http_msg *msg, enum http_hdrid id);
uint32_t http_msg_xhdr_count(const struct http_msg *msg, const char *name);
bool http_msg_hdr_has_value(const struct http_msg *msg, enum http_hdrid id,
			    const char *value);
bool http_msg_xhdr_has_value(const struct http_msg *msg, const char *name,
			     const char *value);
int  http_msg_print(struct re_printf *pf, const struct http_msg *msg);


/* Client */
struct http_cli;
struct http_req;
struct dnsc;

typedef void (http_resp_h)(int err, const struct http_msg *msg, void *arg);
typedef void (http_data_h)(struct mbuf *mb, void *arg);

int http_client_alloc(struct http_cli **clip, struct dnsc *dnsc);
int http_request(struct http_req **reqp, struct http_cli *cli, const char *met,
		 const char *uri, http_resp_h *resph, http_data_h *datah,
		 void *arg, const char *fmt, ...);
struct tcp_conn *http_req_tcp(struct http_req *req);
struct tls_conn *http_req_tls(struct http_req *req);


/* Server */
struct http_sock;
struct http_conn;

typedef void (http_req_h)(struct http_conn *conn, const struct http_msg *msg,
			  void *arg);

int  http_listen(struct http_sock **sockp, const struct sa *laddr,
		 http_req_h *reqh, void *arg);
int  https_listen(struct http_sock **sockp, const struct sa *laddr,
		  const char *cert, http_req_h *reqh, void *arg);
struct tcp_sock *http_sock_tcp(struct http_sock *sock);
const struct sa *http_conn_peer(const struct http_conn *conn);
struct tcp_conn *http_conn_tcp(struct http_conn *conn);
struct tls_conn *http_conn_tls(struct http_conn *conn);
void http_conn_close(struct http_conn *conn);
int  http_reply(struct http_conn *conn, uint16_t scode, const char *reason,
		const char *fmt, ...);
int  http_creply(struct http_conn *conn, uint16_t scode, const char *reason,
		 const char *ctype, const char *fmt, ...);
int  http_ereply(struct http_conn *conn, uint16_t scode, const char *reason);


/* Authentication */
struct http_auth {
	const char *realm;
	bool stale;
};

typedef int (http_auth_h)(const struct pl *username, uint8_t *ha1, void *arg);

int  http_auth_print_challenge(struct re_printf *pf,
			       const struct http_auth *auth);
bool http_auth_check(const struct pl *hval, const struct pl *method,
		     struct http_auth *auth, http_auth_h *authh, void *arg);
bool http_auth_check_request(const struct http_msg *msg,
			     struct http_auth *auth,
			     http_auth_h *authh, void *arg);
