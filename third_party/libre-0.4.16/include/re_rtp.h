/**
 * @file re_rtp.h  Interface to Real-time Transport Protocol and RTCP
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** RTP protocol values */
enum {
	RTP_VERSION     =  2,  /**< Defines the RTP version we support */
	RTCP_VERSION    =  2,  /**< Supported RTCP Version             */
	RTP_HEADER_SIZE = 12   /**< Number of bytes in RTP Header      */
};


/** Defines the RTP header */
struct rtp_header {
	uint8_t  ver;       /**< RTP version number     */
	bool     pad;       /**< Padding bit            */
	bool     ext;       /**< Extension bit          */
	uint8_t  cc;        /**< CSRC count             */
	bool     m;         /**< Marker bit             */
	uint8_t  pt;        /**< Payload type           */
	uint16_t seq;       /**< Sequence number        */
	uint32_t ts;        /**< Timestamp              */
	uint32_t ssrc;      /**< Synchronization source */
	uint32_t csrc[16];  /**< Contributing sources   */
	struct {
		uint16_t type;  /**< Defined by profile     */
		uint16_t len;   /**< Number of 32-bit words */
	} x;
};

/** RTCP Packet Types */
enum rtcp_type {
	RTCP_FIR   = 192,  /**< Full INTRA-frame Request (RFC 2032)    */
	RTCP_NACK  = 193,  /**< Negative Acknowledgement (RFC 2032)    */
	RTCP_SR    = 200,  /**< Sender Report                          */
	RTCP_RR    = 201,  /**< Receiver Report                        */
	RTCP_SDES  = 202,  /**< Source Description                     */
	RTCP_BYE   = 203,  /**< Goodbye                                */
	RTCP_APP   = 204,  /**< Application-defined                    */
	RTCP_RTPFB = 205,  /**< Transport layer FB message (RFC 4585)  */
	RTCP_PSFB  = 206,  /**< Payload-specific FB message (RFC 4585) */
	RTCP_XR    = 207,  /**< Extended Report (RFC 3611)             */
	RTCP_AVB   = 208,  /**< AVB RTCP Packet (IEEE1733)             */
};

/** SDES Types */
enum rtcp_sdes_type {
	RTCP_SDES_END   = 0,  /**< End of SDES list               */
	RTCP_SDES_CNAME = 1,  /**< Canonical name                 */
	RTCP_SDES_NAME  = 2,  /**< User name                      */
	RTCP_SDES_EMAIL = 3,  /**< User's electronic mail address */
	RTCP_SDES_PHONE = 4,  /**< User's phone number            */
	RTCP_SDES_LOC   = 5,  /**< Geographic user location       */
	RTCP_SDES_TOOL  = 6,  /**< Name of application or tool    */
	RTCP_SDES_NOTE  = 7,  /**< Notice about the source        */
	RTCP_SDES_PRIV  = 8   /**< Private extension              */
};

/** Transport Layer Feedback Messages */
enum rtcp_rtpfb {
	RTCP_RTPFB_GNACK = 1  /**< Generic NACK */
};

/** Payload-Specific Feedback Messages */
enum rtcp_psfb {
	RTCP_PSFB_PLI  = 1,   /**< Picture Loss Indication (PLI) */
	RTCP_PSFB_SLI  = 2,   /**< Slice Loss Indication (SLI)   */
	RTCP_PSFB_AFB  = 15,  /**< Application layer Feedback Messages */
};

/** Reception report block */
struct rtcp_rr {
	uint32_t ssrc;            /**< Data source being reported      */
	unsigned int fraction:8;  /**< Fraction lost since last SR/RR  */
	int lost:24;              /**< Cumul. no. pkts lost (signed!)  */
	uint32_t last_seq;        /**< Extended last seq. no. received */
	uint32_t jitter;          /**< Interarrival jitter             */
	uint32_t lsr;             /**< Last SR packet from this source */
	uint32_t dlsr;            /**< Delay since last SR packet      */
};

/** SDES item */
struct rtcp_sdes_item {
	enum rtcp_sdes_type type; /**< Type of item (enum rtcp_sdes_type) */
	uint8_t length;           /**< Length of item (in octets)         */
	char *data;               /**< Text, not null-terminated          */
};

/** One RTCP Message */
struct rtcp_msg {
	/** RTCP Header */
	struct rtcp_hdr {
		unsigned int version:2;  /**< Protocol version       */
		unsigned int p:1;        /**< Padding flag           */
		unsigned int count:5;    /**< Varies by packet type  */
		unsigned int pt:8;       /**< RTCP packet type       */
		uint16_t length;         /**< Packet length in words */
	} hdr;
	union {
		/** Sender report (SR) */
		struct {
			uint32_t ssrc;        /**< Sender generating report  */
			uint32_t ntp_sec;     /**< NTP timestamp - seconds   */
			uint32_t ntp_frac;    /**< NTP timestamp - fractions */
			uint32_t rtp_ts;      /**< RTP timestamp             */
			uint32_t psent;       /**< RTP packets sent          */
			uint32_t osent;       /**< RTP octets sent           */
			struct rtcp_rr *rrv;  /**< Reception report blocks   */
		} sr;

		/** Reception report (RR) */
		struct {
			uint32_t ssrc;        /**< Receiver generating report*/
			struct rtcp_rr *rrv;  /**< Reception report blocks   */
		} rr;

		/** Source Description (SDES) */
		struct rtcp_sdes {
			uint32_t src;         /**< First SSRC/CSRC           */
			struct rtcp_sdes_item *itemv;  /**< SDES items       */
			uint32_t n;           /**< Number of SDES items      */
		} *sdesv;

		/** BYE */
		struct {
			uint32_t *srcv;    /**< List of sources              */
			char *reason;      /**< Reason for leaving (opt.)    */
		} bye;

		/** Application-defined (APP) */
		struct {
			uint32_t src;      /**< SSRC/CSRC                  */
			char name[4];      /**< Name (ASCII)               */
			uint8_t *data;     /**< Application data (32 bits) */
			size_t data_len;   /**< Number of data bytes       */
		} app;

		/** Full INTRA-frame Request (FIR) packet */
		struct {
			uint32_t ssrc;  /**< SSRC for sender of this packet */
		} fir;

		/** Negative ACKnowledgements (NACK) packet */
		struct {
			uint32_t ssrc;  /**< SSRC for sender of this packet */
			uint16_t fsn;   /**< First Sequence Number lost     */
			uint16_t blp;   /**< Bitmask of lost packets        */
		} nack;

		/** Feedback (RTPFB or PSFB) packet */
		struct {
			uint32_t ssrc_packet;
			uint32_t ssrc_media;
			uint32_t n;
			/** Feedback Control Information (FCI) */
			union {
				struct gnack {
					uint16_t pid;
					uint16_t blp;
				} *gnackv;
				struct sli {
					uint16_t first;
					uint16_t number;
					uint8_t picid;
				} *sliv;
				struct mbuf *afb;
				void *p;
			} fci;
		} fb;
	} r;
};

/** RTCP Statistics */
struct rtcp_stats {
	struct {
		uint32_t sent;  /**< Tx RTP Packets                  */
		int lost;       /**< Tx RTP Packets Lost             */
		uint32_t jit;   /**< Tx Inter-arrival Jitter in [us] */
	} tx;
	struct {
		uint32_t sent;  /**< Rx RTP Packets                  */
		int lost;       /**< Rx RTP Packets Lost             */
		uint32_t jit;   /**< Rx Inter-Arrival Jitter in [us] */
	} rx;
	uint32_t rtt;           /**< Current Round-Trip Time in [us] */
};

struct sa;
struct re_printf;
struct rtp_sock;

typedef void (rtp_recv_h)(const struct sa *src, const struct rtp_header *hdr,
			  struct mbuf *mb, void *arg);
typedef void (rtcp_recv_h)(const struct sa *src, struct rtcp_msg *msg,
			   void *arg);

/* RTP api */
int   rtp_alloc(struct rtp_sock **rsp);
int   rtp_listen(struct rtp_sock **rsp, int proto, const struct sa *ip,
		 uint16_t min_port, uint16_t max_port, bool enable_rtcp,
		 rtp_recv_h *recvh, rtcp_recv_h *rtcph, void *arg);
int   rtp_hdr_encode(struct mbuf *mb, const struct rtp_header *hdr);
int   rtp_hdr_decode(struct rtp_header *hdr, struct mbuf *mb);
int   rtp_encode(struct rtp_sock *rs, bool marker, uint8_t pt,
		 uint32_t ts, struct mbuf *mb);
int   rtp_decode(struct rtp_sock *rs, struct mbuf *mb, struct rtp_header *hdr);
int   rtp_send(struct rtp_sock *rs, const struct sa *dst,
	       bool marker, uint8_t pt, uint32_t ts, struct mbuf *mb);
int   rtp_debug(struct re_printf *pf, const struct rtp_sock *rs);
void *rtp_sock(const struct rtp_sock *rs);
uint32_t rtp_sess_ssrc(const struct rtp_sock *rs);
const struct sa *rtp_local(const struct rtp_sock *rs);

/* RTCP session api */
void  rtcp_start(struct rtp_sock *rs, const char *cname,
		 const struct sa *peer);
void  rtcp_enable_mux(struct rtp_sock *rs, bool enabled);
void  rtcp_set_srate(struct rtp_sock *rs, uint32_t sr_tx, uint32_t sr_rx);
void  rtcp_set_srate_tx(struct rtp_sock *rs, uint32_t srate_tx);
void  rtcp_set_srate_rx(struct rtp_sock *rs, uint32_t srate_rx);
int   rtcp_send_app(struct rtp_sock *rs, const char name[4],
		    const uint8_t *data, size_t len);
int   rtcp_send_fir(struct rtp_sock *rs, uint32_t ssrc);
int   rtcp_send_nack(struct rtp_sock *rs, uint16_t fsn, uint16_t blp);
int   rtcp_send_pli(struct rtp_sock *rs, uint32_t fb_ssrc);
int   rtcp_debug(struct re_printf *pf, const struct rtp_sock *rs);
void *rtcp_sock(const struct rtp_sock *rs);
int   rtcp_stats(struct rtp_sock *rs, uint32_t ssrc, struct rtcp_stats *stats);

/* RTCP utils */
int   rtcp_encode(struct mbuf *mb, enum rtcp_type type, uint32_t count, ...);
int   rtcp_decode(struct rtcp_msg **msgp, struct mbuf *mb);
int   rtcp_msg_print(struct re_printf *pf, const struct rtcp_msg *msg);
int   rtcp_sdes_encode(struct mbuf *mb, uint32_t src, uint32_t itemc, ...);
const char *rtcp_type_name(enum rtcp_type type);
const char *rtcp_sdes_name(enum rtcp_sdes_type sdes);
