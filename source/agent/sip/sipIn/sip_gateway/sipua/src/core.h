/**
 * @file core.h  Internal API
 *
 * Copyright (C) 2010 Creytiv.com
 */


/**
 * RFC 3551:
 *
 *    0 -  95  Static payload types
 *   96 - 127  Dynamic payload types
 */
enum {
	PT_CN       = 13,
	PT_STAT_MIN = 0,
	PT_STAT_MAX = 95,
	PT_DYN_MIN  = 96,
	PT_DYN_MAX  = 127
};


/** Media constants */
enum {
	AUDIO_BANDWIDTH = 128000 /**< Bandwidth for audio in bits/s       */
};


/*
 * Account
 */


struct account {
	char *buf;                   /**< Buffer for the SIP address         */
	struct sip_addr laddr;       /**< Decoded SIP address                */
	struct uri luri;             /**< Decoded AOR uri                    */
	char *dispname;              /**< Display name                       */
	char *aor;                   /**< Local SIP uri                      */

	/* parameters: */
	enum answermode answermode;  /**< Answermode for incoming calls      */
	struct le acv[8];            /**< List elements for aucodecl         */
	struct list aucodecl;        /**< List of preferred audio-codecs     */
	char *auth_user;             /**< Authentication username            */
	char *auth_pass;             /**< Authentication password            */
	char *mnatid;                /**< Media NAT handling                 */
	char *mencid;                /**< Media encryption type              */
	const struct mnat *mnat;     /**< MNAT module                        */
	const struct menc *menc;     /**< MENC module                        */
	char *outbound[2];           /**< Optional SIP outbound proxies      */
	uint32_t ptime;              /**< Configured packet time in [ms]     */
	uint32_t regint;             /**< Registration interval in [seconds] */
	uint32_t pubint;             /**< Publication interval in [seconds]  */
	char *regq;                  /**< Registration Q-value               */
	char *rtpkeep;               /**< RTP Keepalive mechanism            */
	char *sipnat;                /**< SIP Nat mechanism                  */
	char *stun_user;             /**< STUN Username                      */
	char *stun_pass;             /**< STUN Password                      */
	char *stun_host;             /**< STUN Hostname                      */
	uint16_t stun_port;          /**< STUN Port number                   */
	struct le vcv[4];            /**< List elements for vidcodecl        */
	struct list vidcodecl;       /**< List of preferred video-codecs     */
};

/*
 * Audio Stream
 */

struct audio;

typedef void (audio_event_h)(int key, bool end, void *arg);
typedef void (audio_err_h)(int err, const char *str, void *arg);

int audio_alloc(struct audio **ap, const struct config *cfg,
		struct call *call, struct sdp_session *sdp_sess, int label,
		const struct mnat *mnat, struct mnat_sess *mnat_sess,
		const struct menc *menc, struct menc_sess *menc_sess,
		uint32_t ptime, const struct list *aucodecl,
		audio_event_h *eventh, audio_err_h *errh, void *arg);
int  audio_start(struct audio *a);
void audio_stop(struct audio *a);
int  audio_encoder_set(struct audio *a, const struct aucodec *ac,
		       int pt_tx, const char *params);
int  audio_decoder_set(struct audio *a, const struct aucodec *ac,
		       int pt_rx, const char *params);
struct stream *audio_strm(const struct audio *a);
int  audio_send_digit(struct audio *a, char key);
void audio_sdp_attr_decode(struct audio *a);
// int  audio_print_rtpstat(struct re_printf *pf, const struct audio *au);
void audio_send(struct audio *a, uint8_t *data, size_t len);
int get_audio_counter(const struct audio *a);
void reset_audio_counter(struct audio *a);


/*
 * BFCP
 */

struct bfcp;
struct uag;
int bfcp_alloc(struct bfcp **bfcpp, struct sdp_session *sdp_sess,
	       const char *proto, bool offerer,
	       const struct mnat *mnat, struct mnat_sess *mnat_sess, struct uag *uag);
int bfcp_start(struct bfcp *bfcp);


/*
 * Call Control
 */

struct call;

/** Call parameters */
struct call_prm {
	enum vidmode vidmode;
	int af;
};

int  call_alloc(struct call **callp, const struct config *cfg,
		struct list *lst,
		const char *local_name, const char *local_uri,
		struct account *acc, struct ua *ua, const struct call_prm *prm,
		const struct sip_msg *msg, struct call *xcall,
		call_event_h *eh, void *arg);
int  call_connect(struct call *call, const struct pl *paddr);
int  call_accept(struct call *call, struct sipsess_sock *sess_sock,
		 const struct sip_msg *msg);
int  call_hangup(struct call *call, uint16_t scode, const char *reason);
int  call_progress(struct call *call);
int  call_answer(struct call *call, uint16_t scode);
int  call_sdp_get(const struct call *call, struct mbuf **descp, bool offer);
int  call_jbuf_stat(struct re_printf *pf, const struct call *call);
int  call_info(struct re_printf *pf, const struct call *call);
int  call_reset_transp(struct call *call);
int  call_notify_sipfrag(struct call *call, uint16_t scode,
			 const char *reason, ...);
int  call_af(const struct call *call);
void call_set_xrtpstat(struct call *call);


/*
 * Conf
 */

int conf_get_range(const struct conf *conf, const char *name,
		   struct range *rng);
int conf_get_csv(const struct conf *conf, const char *name,
		 char *str1, size_t sz1, char *str2, size_t sz2);


/*
 * Media control
 */

int mctrl_handle_media_control(struct pl *body, bool *pfu);


/*
 * Media NAT traversal
 */

struct mnat {
	struct le le;
	const char *id;
	const char *ftag;
	mnat_sess_h *sessh;
	mnat_media_h *mediah;
	mnat_update_h *updateh;
};

const struct mnat *mnat_find(const char *id);


/*
 * Metric
 */

struct metric {
	/* internal stuff: */
	struct tmr tmr;
	uint64_t ts_start;
	bool started;

	/* counters: */
	uint32_t n_packets;
	uint32_t n_bytes;
	uint32_t n_err;

	/* bitrate calculation */
	uint32_t cur_bitrate;
	uint64_t ts_last;
	uint32_t n_bytes_last;
};

void     metric_init(struct metric *metric);
void     metric_reset(struct metric *metric);
void     metric_add_packet(struct metric *metric, size_t packetsize);
uint32_t metric_avg_bitrate(const struct metric *metric);


/*
 * Module
 */

int module_init(const struct conf *conf);
void module_app_unload(void);


/*
 * Network
 */

int net_reset(void);


/*
 * Register client
 */

struct reg;

int  reg_add(struct list *lst, struct ua *ua, int regid);
int  reg_register(struct reg *reg, const char *reg_uri,
		    const char *params, uint32_t regint, const char *outbound);
void reg_unregister(struct reg *reg);
bool reg_isok(const struct reg *reg);
int  reg_sipfd(const struct reg *reg);
int  reg_debug(struct re_printf *pf, const struct reg *reg);
int  reg_status(struct re_printf *pf, const struct reg *reg);


/*
 * RTP keepalive
 */

struct rtpkeep;

int  rtpkeep_alloc(struct rtpkeep **rkp, const char *method, int proto,
		   struct rtp_sock *rtp, struct sdp_media *sdp);
void rtpkeep_refresh(struct rtpkeep *rk, uint32_t ts);


/*
 * SDP
 */

int sdp_decode_multipart(const struct pl *ctype_prm, struct mbuf *mb);
const struct sdp_format *sdp_media_format_cycle(struct sdp_media *m);


/*
 * Stream
 */

struct rtp_header;

enum {STREAM_PRESZ = 4+12}; /* same as RTP_HEADER_SIZE */

typedef void (stream_rtp_h)(const struct rtp_header *hdr, struct mbuf *mb,
			    void *arg);
typedef void (stream_rtcp_h)(struct rtcp_msg *msg, void *arg);

/** Defines a generic media stream */
struct stream {
	struct le le;            /**< Linked list element                   */
	struct config_avt cfg;   /**< Stream configuration                  */
	struct call *call;       /**< Ref. to call object                   */
	struct sdp_media *sdp;   /**< SDP Media line                        */
	struct rtp_sock *rtp;    /**< RTP Socket                            */
	struct rtpkeep *rtpkeep; /**< RTP Keepalive                         */
	struct rtcp_stats rtcp_stats;/**< RTCP statistics                   */
	struct jbuf *jbuf;       /**< Jitter Buffer for incoming RTP        */
	struct mnat_media *mns;  /**< Media NAT traversal state             */
	const struct menc *menc; /**< Media encryption module               */
	struct menc_sess *mencs; /**< Media encryption session state        */
	struct menc_media *mes;  /**< Media Encryption media state          */
	struct metric metric_tx; /**< Metrics for transmit                  */
	struct metric metric_rx; /**< Metrics for receiving                 */
	char *cname;             /**< RTCP Canonical end-point identifier   */
	uint32_t ssrc_rx;        /**< Incoming syncronizing source          */
	uint32_t pseq;           /**< Sequence number for incoming RTP      */
	int pt_enc;              /**< Payload type for encoding             */
	bool rtcp;               /**< Enable RTCP                           */
	bool rtcp_mux;           /**< RTP/RTCP multiplex supported by peer  */
	bool jbuf_started;       /**< True if jitter-buffer was started     */
	stream_rtp_h *rtph;      /**< Stream RTP handler                    */
	stream_rtcp_h *rtcph;    /**< Stream RTCP handler                   */
	void *arg;               /**< Handler argument                      */
};

int  stream_alloc(struct stream **sp, const struct config_avt *cfg,
		  struct call *call, struct sdp_session *sdp_sess,
		  const char *name, int label,
		  const struct mnat *mnat, struct mnat_sess *mnat_sess,
		  const struct menc *menc, struct menc_sess *menc_sess,
		  const char *cname,
		  stream_rtp_h *rtph, stream_rtcp_h *rtcph, void *arg);
struct sdp_media *stream_sdpmedia(const struct stream *s);
int  stream_send(struct stream *s, bool marker, int pt, uint32_t ts,
		 struct mbuf *mb);
void stream_update(struct stream *s);
void stream_update_encoder(struct stream *s, int pt_enc);
int  stream_jbuf_stat(struct re_printf *pf, const struct stream *s);
void stream_hold(struct stream *s, bool hold);
void stream_set_srate(struct stream *s, uint32_t srate_tx, uint32_t srate_rx);
void stream_send_fir(struct stream *s, bool pli);
void stream_send_rtcpfb(struct stream *s, struct mbuf *mb);
void stream_reset(struct stream *s);
void stream_set_bw(struct stream *s, uint32_t bps);
int  stream_debug(struct re_printf *pf, const struct stream *s);
int  stream_print(struct re_printf *pf, const struct stream *s);


/*
 * User-Agent
 */

struct ua;

void         ua_event(struct ua *ua, enum ua_event ev, struct call *call,
		      const char *fmt, ...);
void         ua_printf(const struct ua *ua, const char *fmt, ...);

//TODO intel webrtc
//struct tls  *uag_tls(void);
const char  *uag_allowed_methods(void);


/*
 * Video Stream
 */

struct video;

typedef void (video_err_h)(int err, const char *str, void *arg);

int  video_alloc(struct video **vp, const struct config *cfg,
		 struct call *call, struct sdp_session *sdp_sess, int label,
		 const struct mnat *mnat, struct mnat_sess *mnat_sess,
		 const struct menc *menc, struct menc_sess *menc_sess,
		 const char *content, const struct list *vidcodecl,
		 video_err_h *errh, void *arg);
int  video_start(struct video *v, const char *peer);
void video_stop(struct video *v);
int  video_encoder_set(struct video *v, struct vidcodec *vc,
		       int pt_tx, const char *params);
int  video_decoder_set(struct video *v, struct vidcodec *vc, int pt_rx,
		       const char *fmtp);
struct stream *video_strm(const struct video *v);
void video_update_picture(struct video *v);
void video_sdp_attr_decode(struct video *v);
int  video_print(struct re_printf *pf, const struct video *v);

void video_send(struct video *v, uint8_t *data, size_t len);
void video_rtcpfb_send(struct video *v, uint8_t *data, size_t len);
void video_fir(struct video *v);
int get_video_counter(const struct video *video);
void reset_video_counter(struct video *video);



/** Defines a SIP User Agent object */
struct ua {
	struct uag *owner;           /**< Belong bo which UAG                */
	struct ua **uap;             /**< Pointer to application's ua        */
	struct le le;                /**< Linked list element                */
	struct account *acc;         /**< Account Parameters                 */
	struct list regl;            /**< List of Register clients           */
	struct list calls;           /**< List of active calls (struct call) */
	struct play *play;           /**< Playback of ringtones etc.         */
	struct pl extensionv[8];     /**< Vector of SIP extensions           */
	size_t    extensionc;        /**< Number of SIP extensions           */
	char *cuser;                 /**< SIP Contact username               */
	int af;                      /**< Preferred Address Family           */
        enum presence_status my_status; /**< Presence Status                 */
};

struct ua_eh {
	struct le le;
	ua_event_h *h;
	void *arg;
};

struct uag {
	void *ep;                      /**< gateway object                  */
	struct config_sip *cfg;        /**< SIP configuration               */
	struct list ehl;               /**< Event handlers (struct ua_eh)   */
	struct sip *sip;               /**< SIP Stack                       */
	struct sip_lsnr *lsnr;         /**< SIP Listener                    */
	struct sipsess_sock *sock;     /**< SIP Session socket              */
	struct sipevent_sock *evsock;  /**< SIP Event socket                */
	struct ua *ua_cur;             /**< Current User-Agent              */
	bool use_udp;                  /**< Use UDP transport               */
	bool use_tcp;                  /**< Use TCP transport               */
	bool use_tls;                  /**< Use TLS transport               */
	bool prefer_ipv6;              /**< Force IPv6 transport            */
#ifdef USE_TLS
	struct tls *tls;               /**< TLS Context                     */
#endif
};

