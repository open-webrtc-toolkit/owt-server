/**
 * @file re_sdp.h  Interface to Session Description Protocol (SDP)
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	SDP_VERSION = 0
};

/** SDP Direction */
enum sdp_dir {
	SDP_INACTIVE = 0,
	SDP_RECVONLY = 1,
	SDP_SENDONLY = 2,
	SDP_SENDRECV = 3,
};

/** SDP Bandwidth type */
enum sdp_bandwidth {
	SDP_BANDWIDTH_MIN = 0,
	SDP_BANDWIDTH_CT = 0,  /**< [kbit/s] Conference Total         */
	SDP_BANDWIDTH_AS,      /**< [kbit/s] Application Specific     */
	SDP_BANDWIDTH_RS,      /**< [bit/s] RTCP Senders (RFC 3556)   */
	SDP_BANDWIDTH_RR,      /**< [bit/s] RTCP Receivers (RFC 3556) */
	SDP_BANDWIDTH_TIAS,    /**< [bit/s] Transport Independent Application
				  Specific Maximum (RFC 3890) */
	SDP_BANDWIDTH_MAX,
};


struct sdp_format;

typedef int(sdp_media_enc_h)(struct mbuf *mb, bool offer, void *arg);
typedef int(sdp_fmtp_enc_h)(struct mbuf *mb, const struct sdp_format *fmt,
			    bool offer, void *data);
typedef bool(sdp_fmtp_cmp_h)(const char *params1, const char *params2,
			     void *data);
typedef bool(sdp_format_h)(struct sdp_format *fmt, void *arg);
typedef bool(sdp_attr_h)(const char *name, const char *value, void *arg);

/** SDP Format */
struct sdp_format {
	struct le le;
	char *id;
	char *params;
	char *rparams;
	char *name;
	sdp_fmtp_enc_h *ench;
	sdp_fmtp_cmp_h *cmph;
	void *data;
	bool ref;
	bool sup;
	int pt;
	uint32_t srate;
	uint8_t ch;
};


/* session */
struct sdp_session;

int  sdp_session_alloc(struct sdp_session **sessp, const struct sa *laddr);
void sdp_session_set_laddr(struct sdp_session *sess, const struct sa *laddr);
void sdp_session_set_lbandwidth(struct sdp_session *sess,
				enum sdp_bandwidth type, int32_t bw);
int  sdp_session_set_lattr(struct sdp_session *sess, bool replace,
			   const char *name, const char *value, ...);
void sdp_session_del_lattr(struct sdp_session *sess, const char *name);
int32_t sdp_session_lbandwidth(const struct sdp_session *sess,
			       enum sdp_bandwidth type);
int32_t sdp_session_rbandwidth(const struct sdp_session *sess,
			       enum sdp_bandwidth type);
const char *sdp_session_rattr(const struct sdp_session *sess,
			      const char *name);
const char *sdp_session_rattr_apply(const struct sdp_session *sess,
				    const char *name,
				    sdp_attr_h *attrh, void *arg);
const struct list *sdp_session_medial(const struct sdp_session *sess,
				      bool local);
int  sdp_session_debug(struct re_printf *pf, const struct sdp_session *sess);


/* media */
struct sdp_media;

int  sdp_media_add(struct sdp_media **mp, struct sdp_session *sess,
		   const char *name, uint16_t port, const char *proto);
int  sdp_media_set_alt_protos(struct sdp_media *m, unsigned protoc, ...);
void sdp_media_set_encode_handler(struct sdp_media *m, sdp_media_enc_h *ench,
				  void *arg);
void sdp_media_set_fmt_ignore(struct sdp_media *m, bool fmt_ignore);
void sdp_media_set_disabled(struct sdp_media *m, bool disabled);
void sdp_media_set_lport(struct sdp_media *m, uint16_t port);
void sdp_media_set_laddr(struct sdp_media *m, const struct sa *laddr);
void sdp_media_set_lbandwidth(struct sdp_media *m, enum sdp_bandwidth type,
			      int32_t bw);
void sdp_media_set_lport_rtcp(struct sdp_media *m, uint16_t port);
void sdp_media_set_laddr_rtcp(struct sdp_media *m, const struct sa *laddr);
void sdp_media_set_ldir(struct sdp_media *m, enum sdp_dir dir);
int  sdp_media_set_lattr(struct sdp_media *m, bool replace,
			 const char *name, const char *value, ...);
void sdp_media_del_lattr(struct sdp_media *m, const char *name);
const char *sdp_media_proto(const struct sdp_media *m);
uint16_t sdp_media_rport(const struct sdp_media *m);
const struct sa *sdp_media_raddr(const struct sdp_media *m);
const struct sa *sdp_media_laddr(const struct sdp_media *m);
void sdp_media_raddr_rtcp(const struct sdp_media *m, struct sa *raddr);
int32_t sdp_media_rbandwidth(const struct sdp_media *m,
			     enum sdp_bandwidth type);
enum sdp_dir sdp_media_ldir(const struct sdp_media *m);
enum sdp_dir sdp_media_rdir(const struct sdp_media *m);
enum sdp_dir sdp_media_dir(const struct sdp_media *m);
const struct sdp_format *sdp_media_lformat(const struct sdp_media *m, int pt);
const struct sdp_format *sdp_media_rformat(const struct sdp_media *m,
					   const char *name);
struct sdp_format *sdp_media_format(const struct sdp_media *m,
				    bool local, const char *id,
				    int pt, const char *name,
				    int32_t srate, int8_t ch);
struct sdp_format *sdp_media_format_apply(const struct sdp_media *m,
					  bool local, const char *id,
					  int pt, const char *name,
					  int32_t srate, int8_t ch,
					  sdp_format_h *fmth, void *arg);
const struct list *sdp_media_format_lst(const struct sdp_media *m, bool local);
const char *sdp_media_rattr(const struct sdp_media *m, const char *name);
const char *sdp_media_session_rattr(const struct sdp_media *m,
				    const struct sdp_session *sess,
				    const char *name);
const char *sdp_media_rattr_apply(const struct sdp_media *m, const char *name,
				  sdp_attr_h *attrh, void *arg);
const char *sdp_media_name(const struct sdp_media *m);
int  sdp_media_debug(struct re_printf *pf, const struct sdp_media *m);


/* format */
int  sdp_format_add(struct sdp_format **fmtp, struct sdp_media *m,
		    bool prepend, const char *id, const char *name,
		    uint32_t srate, uint8_t ch, sdp_fmtp_enc_h *ench,
		    sdp_fmtp_cmp_h *cmph, void *data, bool ref,
		    const char *params, ...);
int  sdp_format_set_params(struct sdp_format *fmt, const char *params, ...);
bool sdp_format_cmp(const struct sdp_format *fmt1,
		    const struct sdp_format *fmt2);
int  sdp_format_debug(struct re_printf *pf, const struct sdp_format *fmt);


/* encode/decode */
int sdp_encode(struct mbuf **mbp, struct sdp_session *sess, bool offer);
int sdp_decode(struct sdp_session *sess, struct mbuf *mb, bool offer);


/* strings */
const char *sdp_dir_name(enum sdp_dir dir);
const char *sdp_bandwidth_name(enum sdp_bandwidth type);


extern const char sdp_attr_fmtp[];
extern const char sdp_attr_maxptime[];
extern const char sdp_attr_ptime[];
extern const char sdp_attr_rtcp[];
extern const char sdp_attr_rtpmap[];

extern const char sdp_media_audio[];
extern const char sdp_media_video[];
extern const char sdp_media_text[];

extern const char sdp_proto_rtpavp[];
extern const char sdp_proto_rtpsavp[];


/* utility functions */

/** RTP Header Extensions, as defined in RFC 5285 */
struct sdp_extmap {
	struct pl name;
	struct pl attrs;
	enum sdp_dir dir;
	bool dir_set;
	uint32_t id;
};

int sdp_extmap_decode(struct sdp_extmap *ext, const char *val);
