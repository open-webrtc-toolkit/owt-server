/**
 * @file baresip.h  Public Interface to Baresip
 *
 * Copyright (C) 2010 Creytiv.com
 */

#ifndef BARESIP_H__
#define BARESIP_H__

#ifdef __cplusplus
extern "C" {
#endif


/** Defines the Baresip version string */
#define SIPUA_VERSION "0.4.19"


/* forward declarations */
struct sa;
struct sdp_media;
struct sdp_session;
struct sip_msg;
struct ua;
struct vidframe;
struct vidrect;
struct vidsz;


/*
 * Account
 */

/** Defines the answermodes */
enum answermode {
	ANSWERMODE_MANUAL = 0,
	ANSWERMODE_EARLY,
	ANSWERMODE_AUTO
};

struct account;

int account_alloc(struct account **accp, const char *sipaddr);
int account_debug(struct re_printf *pf, const struct account *acc);
int account_set_display_name(struct account *acc, const char *dname);
int account_auth(const struct account *acc, char **username, char **password,
		 const char *realm);
struct list *account_aucodecl(const struct account *acc);
struct list *account_vidcodecl(const struct account *acc);
struct sip_addr *account_laddr(const struct account *acc);
uint32_t account_regint(const struct account *acc);
uint32_t account_pubint(const struct account *acc);
enum answermode account_answermode(const struct account *acc);


/*
 * Call
 */

enum call_event {
	CALL_EVENT_INCOMING,
	CALL_EVENT_RINGING,
	CALL_EVENT_PROGRESS,
	CALL_EVENT_ESTABLISHED,
	CALL_EVENT_CLOSED,
	CALL_EVENT_TRANSFER,
	CALL_EVENT_TRANSFER_FAILED,
	CALL_EVENT_UPDATE,
};

struct call;

typedef void (call_event_h)(struct call *call, enum call_event ev,
			    const char *str, void *arg);
typedef void (call_dtmf_h)(struct call *call, char key, void *arg);

int  call_modify(struct call *call);
int  call_hold(struct call *call, bool hold);
int  call_send_digit(struct call *call, char key);
bool call_has_audio(const struct call *call);
bool call_has_video(const struct call *call);
int  call_transfer(struct call *call, const char *uri);
int  call_status(struct re_printf *pf, const struct call *call);
int  call_debug(struct re_printf *pf, const struct call *call);
void call_set_handlers(struct call *call, call_event_h *eh,
		       call_dtmf_h *dtmfh, void *arg);
uint16_t      call_scode(const struct call *call);
uint32_t      call_duration(const struct call *call);
const char   *call_peeruri(const struct call *call);
const char   *call_peername(const struct call *call);
const char   *call_localuri(const struct call *call);
struct audio *call_audio(const struct call *call);
struct video *call_video(const struct call *call);
struct list  *call_streaml(const struct call *call);
struct ua    *call_get_ua(const struct call *call);
bool          call_is_onhold(const struct call *call);
bool          call_is_outgoing(const struct call *call);

void *call_get_owner(const struct call *call);
void call_set_owner(struct call *call, void *owner);
const char *call_audio_dir(const struct call *call);
const char *call_video_dir(const struct call *call);
void call_subscribe_audio(struct call *call, void *subscriber);
void call_subscribe_video(struct call *call, void *subscriber);
void call_connection_tx_audio(void* call, uint8_t *data, size_t len);
void call_connection_tx_video(void* call, uint8_t *data, size_t len);
void call_connection_fir(void* call);

/*
 * Conf (utils)
 */


/*
 * Config (core configuration)
 */

/** A range of numbers */
struct range {
	uint32_t min;  /**< Minimum number */
	uint32_t max;  /**< Maximum number */
};

static inline bool in_range(const struct range *rng, uint32_t val)
{
	return rng ? (val >= rng->min && val <= rng->max) : false;
}

/** Audio transmit mode */
enum audio_mode {
	AUDIO_MODE_POLL = 0,         /**< Polling mode                  */
	AUDIO_MODE_THREAD,           /**< Use dedicated thread          */
	AUDIO_MODE_THREAD_REALTIME,  /**< Use dedicated realtime-thread */
	AUDIO_MODE_TMR               /**< Use timer                     */
};


/** SIP User-Agent */
struct config_sip {
	uint32_t trans_bsize;   /**< SIP Transaction bucket size    */
	char uuid[64];          /**< Universally Unique Identifier  */
	char local[64];         /**< Local SIP Address              */
	char cert[256];         /**< SIP Certificate                */
};

/** Audio */
struct config_audio {
	struct range srate;     /**< Audio sampling rate in [Hz]    */
	struct range channels;  /**< Nr. of audio channels (1=mono) */
};

#ifdef USE_VIDEO
/** Video */
struct config_video {
	unsigned width, height; /**< Video resolution               */
	uint32_t bitrate;       /**< Encoder bitrate in [bit/s]     */
	uint32_t fps;           /**< Video framerate                */
};
#endif

/** Audio/Video Transport */
struct config_avt {
	uint8_t rtp_tos;        /**< Type-of-Service for outg. RTP  */
	struct range rtp_ports; /**< RTP port range                 */
	struct range rtp_bw;    /**< RTP Bandwidth range [bit/s]    */
	bool rtcp_enable;       /**< RTCP is enabled                */
	bool rtcp_mux;          /**< RTP/RTCP multiplexing          */
	struct range jbuf_del;  /**< Delay, number of frames        */
	bool rtp_stats;         /**< Enable RTP statistics          */
};

/* Network */
struct config_net {
	char ifname[16];        /**< Bind to interface (optional)   */
};

#ifdef USE_VIDEO
/* BFCP */
struct config_bfcp {
	char proto[16];         /**< BFCP Transport (optional)      */
};
#endif


/** Core configuration */
struct config {

	struct config_sip sip;

	struct config_audio audio;

#ifdef USE_VIDEO
	struct config_video video;
#endif
	struct config_avt avt;

	struct config_net net;

#ifdef USE_VIDEO
	struct config_bfcp bfcp;
#endif
};
int conf_init(void);
int load_modules(void/*const char *modpath*/);
void unload_modules(void);
struct config *conf_config(void);


/*
 * Contact
 */

enum presence_status {
	PRESENCE_UNKNOWN,
	PRESENCE_OPEN,
	PRESENCE_CLOSED,
	PRESENCE_BUSY
};

/*
 * Media Context
 */

/** Media Context */
struct media_ctx {
	const char *id;  /**< Media Context identifier */
};

/*
 * Log
 */

enum log_level {
	LEVEL_DEBUG = 0,
	LEVEL_INFO,
	LEVEL_WARN,
	LEVEL_ERROR,
};

typedef void (log_h)(uint32_t level, const char *msg);

struct log {
	struct le le;
	log_h *h;
};

void log_register_handler(struct log *log);
void log_unregister_handler(struct log *log);
void log_enable_debug(bool enable);
void log_enable_info(bool enable);
void log_enable_stderr(bool enable);
void vlog(enum log_level level, const char *fmt, va_list ap);
void loglv(enum log_level level, const char *fmt, ...);
void debug(const char *fmt, ...);
void info(const char *fmt, ...);
void warning(const char *fmt, ...);
void error(const char *fmt, ...);


/*
 * Menc - Media encryption (for RTP)
 */

struct menc;
struct menc_sess;
struct menc_media;


typedef void (menc_error_h)(int err, void *arg);

typedef int  (menc_sess_h)(struct menc_sess **sessp, struct sdp_session *sdp,
			   bool offerer, menc_error_h *errorh, void *arg);

typedef int  (menc_media_h)(struct menc_media **mp, struct menc_sess *sess,
			    struct rtp_sock *rtp, int proto,
			    void *rtpsock, void *rtcpsock,
			    struct sdp_media *sdpm);

struct menc {
	struct le le;
	const char *id;
	const char *sdp_proto;
	menc_sess_h *sessh;
	menc_media_h *mediah;
};

void menc_register(struct menc *menc);
void menc_unregister(struct menc *menc);
const struct menc *menc_find(const char *id);


/*
 * Net - Networking
 */

typedef void (net_change_h)(void *arg);

int  net_init(const struct config_net *cfg, int af);
void net_close(void);
int  net_dnssrv_add(const struct sa *sa);
void net_change(uint32_t interval, net_change_h *ch, void *arg);
bool net_check(void);
int  net_af(void);
int  net_debug(struct re_printf *pf, void *unused);
const struct sa *net_laddr_af(int af);
const char      *net_domain(void);
struct dnsc     *net_dnsc(void);

/*
 * User Agent
 */

struct ua;
struct uag;

/** Events from User-Agent */
enum ua_event {
	UA_EVENT_REGISTERING = 0,
	UA_EVENT_REGISTER_OK,
	UA_EVENT_REGISTER_FAIL,
	UA_EVENT_UNREGISTERING,
	UA_EVENT_SHUTDOWN,
	UA_EVENT_EXIT,

	UA_EVENT_CALL_INCOMING,
	UA_EVENT_CALL_RINGING,
	UA_EVENT_CALL_PROGRESS,
	UA_EVENT_CALL_ESTABLISHED,
	UA_EVENT_CALL_CLOSED,
	UA_EVENT_CALL_TRANSFER_FAILED,
	UA_EVENT_CALL_DTMF_START,
	UA_EVENT_CALL_DTMF_END,

	UA_EVENT_MAX,
};

/** Video mode */
enum vidmode {
	VIDMODE_OFF = 0,    /**< Video disabled                */
	VIDMODE_ON,         /**< Video enabled                 */
};

/** Defines the User-Agent event handler */
typedef void (ua_event_h)(struct ua *ua, enum ua_event ev,
			  struct call *call, const char *prm, void *arg);
typedef void (options_resp_h)(int err, const struct sip_msg *msg, void *arg);

/* Multiple instances */
int  ua_alloc(struct ua **uap, const char *aor, struct uag *uag);
int  ua_connect(struct ua *ua, struct call **callp,
		const char *from_uri, const char *uri,
		const char *params, enum vidmode vmode);
void ua_hangup(struct ua *ua, struct call *call,
	       uint16_t scode, const char *reason);
int  ua_answer(struct ua *ua, struct call *call);
int  ua_hold_answer(struct ua *ua, struct call *call);
int  ua_options_send(struct ua *ua, const char *uri,
		     options_resp_h *resph, void *arg);
int  ua_sipfd(const struct ua *ua);
int  ua_debug(struct re_printf *pf, const struct ua *ua);
int  ua_print_calls(struct re_printf *pf, const struct ua *ua);
int  ua_print_status(struct re_printf *pf, const struct ua *ua);
int  ua_print_supported(struct re_printf *pf, const struct ua *ua);
int  ua_register(struct ua *ua);
void ua_unregister(struct ua *ua);
bool ua_isregistered(const struct ua *ua);
void ua_pub_gruu_set(struct ua *ua, const struct pl *pval);
const char     *ua_aor(const struct ua *ua);
const char     *ua_cuser(const struct ua *ua);
const char     *ua_local_cuser(const struct ua *ua);
struct account *ua_account(const struct ua *ua);
const char     *ua_outbound(const struct ua *ua);
struct call    *ua_call(const struct ua *ua);
struct call    *ua_prev_call(const struct ua *ua);
struct account *ua_prm(const struct ua *ua);
struct list    *ua_calls(const struct ua *ua);
enum presence_status ua_presence_status(const struct ua *ua);
void ua_presence_status_set(struct ua *ua, const enum presence_status status);
void ua_set_media_af(struct ua *ua, int af_media);

void ua_handle_options(struct ua *ua, const struct sip_msg *msg);
int ua_call_alloc(struct call **callp, struct ua *ua,
                         enum vidmode vidmode, const struct sip_msg *msg,
                         struct call *xcall, const char *local_uri);

/* One instance */
int  uag_alloc(struct uag **uagp, const char *software, void *gateway, bool udp, bool tcp, bool tls,
	     bool prefer_ipv6);
void uag_close(struct uag *uag);
void uag_stop_all(struct uag *uag);
int  uag_reset_transp(struct uag *uag, bool reg, bool reinvite);
int  uag_event_register(struct uag *uag, ua_event_h *eh, void *arg);
void uag_event_unregister(struct uag *uag, ua_event_h *eh);


int  uag_print_sip_status(const struct uag *uag, struct re_printf *pf, void *unused);

struct ua   *uag_find(const struct uag *uag, const struct pl *cuser);
struct ua   *uag_find_aor(const struct uag *uag, const char *aor);
struct ua   *uag_find_param(const struct uag *uag, const char *name, const char *val);
const char  *uag_event_str(enum ua_event ev);
void         uag_current_set(struct uag *uag, struct ua *ua);
struct ua   *uag_current(struct uag *uag);
const char *uag_allowed_methods(void);


void uag_set_sub_handler(sip_msg_h *subh);
int  uag_set_extra_params(const char *eprm);


/*
 * Audio Codec
 */

/** Audio Codec parameters */
struct auenc_param {
	uint32_t ptime;  /**< Packet time in [ms]   */
};

struct auenc_state;
struct audec_state;
struct aucodec;

typedef int (auenc_update_h)(struct auenc_state **aesp,
			     const struct aucodec *ac,
			     struct auenc_param *prm, const char *fmtp);
typedef int (auenc_encode_h)(struct auenc_state *aes, uint8_t *buf,
			     size_t *len, const int16_t *sampv, size_t sampc);

typedef int (audec_update_h)(struct audec_state **adsp,
			     const struct aucodec *ac, const char *fmtp);
typedef int (audec_decode_h)(struct audec_state *ads, int16_t *sampv,
			     size_t *sampc, const uint8_t *buf, size_t len);
typedef int (audec_plc_h)(struct audec_state *ads,
			  int16_t *sampv, size_t *sampc);

struct aucodec {
	struct le le;
	const char *pt;
	const char *name;
	uint32_t srate;             /* Audio samplerate */
	// uint32_t crate;             /* RTP Clock rate   */
	uint8_t ch;
	const char *fmtp;
	auenc_update_h *encupdh;
	auenc_encode_h *ench;
	audec_update_h *decupdh;
	audec_decode_h *dech;
	audec_plc_h    *plch;
	sdp_fmtp_enc_h *fmtp_ench;
	sdp_fmtp_cmp_h *fmtp_cmph;
};

void aucodec_register(struct aucodec *ac);
void aucodec_unregister(struct aucodec *ac);
const struct aucodec *aucodec_find(const char *name, uint32_t srate,
				   uint8_t ch);
struct list *aucodec_list(void);
void register_audio_codecs(void);


/*
 * Video Codec
 */

/** Video Codec parameters */
struct videnc_param {
	unsigned bitrate;  /**< Encoder bitrate in [bit/s] */
	unsigned pktsize;  /**< RTP packetsize in [bytes]  */
	unsigned fps;      /**< Video framerate            */
	uint32_t max_fs;
};

struct videnc_state;
struct viddec_state;
struct vidcodec;

typedef int (videnc_packet_h)(bool marker, const uint8_t *hdr, size_t hdr_len,
			      const uint8_t *pld, size_t pld_len, void *arg);

typedef int (videnc_update_h)(struct videnc_state **vesp,
			      const struct vidcodec *vc,
			      struct videnc_param *prm, const char *fmtp,
			      videnc_packet_h *pkth, void *arg);
typedef int (videnc_encode_h)(struct videnc_state *ves, bool update,
			      const struct vidframe *frame);

typedef int (viddec_update_h)(struct viddec_state **vdsp,
			      const struct vidcodec *vc, const char *fmtp);
typedef int (viddec_decode_h)(struct viddec_state *vds, struct vidframe *frame,
			      bool marker, uint16_t seq, struct mbuf *mb);

struct vidcodec {
	struct le le;
	const char *pt;
	const char *name;
	const char *variant;
	const char *fmtp;
	videnc_update_h *encupdh;
	videnc_encode_h *ench;
	viddec_update_h *decupdh;
	viddec_decode_h *dech;
	sdp_fmtp_enc_h *fmtp_ench;
	sdp_fmtp_cmp_h *fmtp_cmph;
};

void vidcodec_register(struct vidcodec *vc);
void vidcodec_unregister(struct vidcodec *vc);
const struct vidcodec *vidcodec_find(const char *name, const char *variant);
const struct vidcodec *vidcodec_find_encoder(const char *name);
const struct vidcodec *vidcodec_find_decoder(const char *name);
struct list *vidcodec_list(void);
void register_video_codecs(void);



/*
 * Audio stream
 */

struct audio;

void audio_mute(struct audio *a, bool muted);
bool audio_ismuted(const struct audio *a);
void audio_set_devicename(struct audio *a, const char *src, const char *play);
int  audio_set_source(struct audio *au, const char *mod, const char *device);
int  audio_set_player(struct audio *au, const char *mod, const char *device);
void audio_encoder_cycle(struct audio *audio);
int  audio_debug(struct re_printf *pf, const struct audio *a);


/*
 * Video stream
 */

struct video;

void  video_mute(struct video *v, bool muted);
void *video_view(const struct video *v);
int   video_set_fullscreen(struct video *v, bool fs);
int   video_set_orient(struct video *v, int orient);
void  video_vidsrc_set_device(struct video *v, const char *dev);
int   video_set_source(struct video *v, const char *name, const char *dev);
void  video_set_devicename(struct video *v, const char *src, const char *disp);
void  video_encoder_cycle(struct video *video);
int   video_debug(struct re_printf *pf, const struct video *v);


/*
 * Media NAT
 */

struct mnat;
struct mnat_sess;
struct mnat_media;

typedef void (mnat_estab_h)(int err, uint16_t scode, const char *reason,
			    void *arg);

typedef int (mnat_sess_h)(struct mnat_sess **sessp, struct dnsc *dnsc,
			  int af, const char *srv, uint16_t port,
			  const char *user, const char *pass,
			  struct sdp_session *sdp, bool offerer,
			  mnat_estab_h *estabh, void *arg);

typedef int (mnat_media_h)(struct mnat_media **mp, struct mnat_sess *sess,
			   int proto, void *sock1, void *sock2,
			   struct sdp_media *sdpm);

typedef int (mnat_update_h)(struct mnat_sess *sess);

int mnat_register(struct mnat **mnatp, const char *id, const char *ftag,
		  mnat_sess_h *sessh, mnat_media_h *mediah,
		  mnat_update_h *updateh);


/*
 * Real-time
 */
int realtime_enable(bool enable, int fps);


/*
 * SDP
 */
bool sdp_media_has_media(const struct sdp_media *m);
int  sdp_media_find_unused_pt(const struct sdp_media *m);
int  sdp_fingerprint_decode(const char *attr, struct pl *hash,
			    uint8_t *md, size_t *sz);
uint32_t sdp_media_rattr_u32(const struct sdp_media *sdpm, const char *name);
const char *sdp_rattr(const struct sdp_session *s, const struct sdp_media *m,
		      const char *name);


/*
 * SIP Request
 */

int sip_req_send(struct ua *ua, const char *method, const char *uri,
		 sip_resp_h *resph, void *arg, const char *fmt, ...);


/*
 * H.264
 */

/** NAL unit types (RFC 3984, Table 1) */
enum {
	H264_NAL_UNKNOWN      = 0,
	/* 1-23   NAL unit  Single NAL unit packet per H.264 */
	H264_NAL_SLICE        = 1,
	H264_NAL_DPA          = 2,
	H264_NAL_DPB          = 3,
	H264_NAL_DPC          = 4,
	H264_NAL_IDR_SLICE    = 5,
	H264_NAL_SEI          = 6,
	H264_NAL_SPS          = 7,
	H264_NAL_PPS          = 8,
	H264_NAL_AUD          = 9,
	H264_NAL_END_SEQUENCE = 10,
	H264_NAL_END_STREAM   = 11,
	H264_NAL_FILLER_DATA  = 12,
	H264_NAL_SPS_EXT      = 13,
	H264_NAL_AUX_SLICE    = 19,

	H264_NAL_STAP_A       = 24,  /**< Single-time aggregation packet */
	H264_NAL_STAP_B       = 25,  /**< Single-time aggregation packet */
	H264_NAL_MTAP16       = 26,  /**< Multi-time aggregation packet  */
	H264_NAL_MTAP24       = 27,  /**< Multi-time aggregation packet  */
	H264_NAL_FU_A         = 28,  /**< Fragmentation unit             */
	H264_NAL_FU_B         = 29,  /**< Fragmentation unit             */
};

/**
 * H.264 Header defined in RFC 3984
 *
 * <pre>
      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+
 * </pre>
 */
struct h264_hdr {
	unsigned f:1;      /**< 1 bit  - Forbidden zero bit (must be 0) */
	unsigned nri:2;    /**< 2 bits - nal_ref_idc                    */
	unsigned type:5;   /**< 5 bits - nal_unit_type                  */
};

int h264_hdr_encode(const struct h264_hdr *hdr, struct mbuf *mb);
int h264_hdr_decode(struct h264_hdr *hdr, struct mbuf *mb);

/** Fragmentation Unit header */
struct h264_fu {
	unsigned s:1;      /**< Start bit                               */
	unsigned e:1;      /**< End bit                                 */
	unsigned r:1;      /**< The Reserved bit MUST be equal to 0     */
	unsigned type:5;   /**< The NAL unit payload type               */
};

int h264_fu_hdr_encode(const struct h264_fu *fu, struct mbuf *mb);
int h264_fu_hdr_decode(struct h264_fu *fu, struct mbuf *mb);

const uint8_t *h264_find_startcode(const uint8_t *p, const uint8_t *end);

int h264_packetize(const uint8_t *buf, size_t len, size_t pktsize,
		   videnc_packet_h *pkth, void *arg);
int h264_nal_send(bool first, bool last,
		  bool marker, uint32_t ihdr, const uint8_t *buf,
		  size_t size, size_t maxsz,
		  videnc_packet_h *pkth, void *arg);
static inline bool h264_is_keyframe(int type)
{
	return type == H264_NAL_SPS;
}


/*
 * Modules
 */

#ifdef STATIC
#define DECL_EXPORTS(name) exports_ ##name
#else
#define DECL_EXPORTS(name) exports
#endif


int module_preload(const char *module);


/*
 * MOS (Mean Opinion Score)
 */

double mos_calculate(double *r_factor, double rtt,
		     double jitter, uint32_t num_packets_lost);


#ifdef __cplusplus
}
#endif


#endif /* BARESIP_H__ */
