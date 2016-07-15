/**
 * @file sdp.h  Internal SDP interface
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	RTP_DYNPT_START =  96,
	RTP_DYNPT_END   = 127,
};


struct sdp_session {
	struct list lmedial;
	struct list medial;
	struct list lattrl;
	struct list rattrl;
	struct sa laddr;
	struct sa raddr;
	int32_t lbwv[SDP_BANDWIDTH_MAX];
	int32_t rbwv[SDP_BANDWIDTH_MAX];
	uint32_t id;
	uint32_t ver;
	enum sdp_dir rdir;
};

struct sdp_media {
	struct le le;
	struct list lfmtl;
	struct list rfmtl;
	struct list lattrl;
	struct list rattrl;
	struct sa laddr;
	struct sa raddr;
	struct sa laddr_rtcp;
	struct sa raddr_rtcp;
	int32_t lbwv[SDP_BANDWIDTH_MAX];
	int32_t rbwv[SDP_BANDWIDTH_MAX];
	char *name;
	char *proto;
	char *protov[8];
	char *uproto;           /* unsupported protocol */
	sdp_media_enc_h *ench;
	void *arg;
	enum sdp_dir ldir;
	enum sdp_dir rdir;
	bool fmt_ignore;
	bool disabled;
	int dynpt;
};


/* session */
void sdp_session_rreset(struct sdp_session *sess);


/* media */
int  sdp_media_radd(struct sdp_media **mp, struct sdp_session *sess,
		    const struct pl *name, const struct pl *proto);
void sdp_media_rreset(struct sdp_media *m);
bool sdp_media_proto_cmp(struct sdp_media *m, const struct pl *proto,
			 bool update);
struct sdp_media *sdp_media_find(const struct sdp_session *sess,
				 const struct pl *name,
				 const struct pl *proto,
				 bool update_proto);
void sdp_media_align_formats(struct sdp_media *m, bool offer);


/* format */
int  sdp_format_radd(struct sdp_media *m, const struct pl *id);
struct sdp_format *sdp_format_find(const struct list *lst,
				   const struct pl *id);


/* attribute */
struct sdp_attr;

int  sdp_attr_add(struct list *lst, struct pl *name, struct pl *val);
int  sdp_attr_addv(struct list *lst, const char *name, const char *val,
		   va_list ap);
void sdp_attr_del(const struct list *lst, const char *name);
const char *sdp_attr_apply(const struct list *lst, const char *name,
			   sdp_attr_h *attrh, void *arg);
int sdp_attr_print(struct re_printf *pf, const struct sdp_attr *attr);
int sdp_attr_debug(struct re_printf *pf, const struct sdp_attr *attr);
