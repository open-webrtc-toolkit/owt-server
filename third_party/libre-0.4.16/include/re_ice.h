/**
 * @file re_ice.h  Interface to Interactive Connectivity Establishment (ICE)
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** ICE mode */
enum ice_mode {
	ICE_MODE_FULL,
	ICE_MODE_LITE
};

/** ICE Component ID */
enum ice_compid {
	ICE_COMPID_RTP  = 1,
	ICE_COMPID_RTCP = 2
};

/** ICE Nomination */
enum ice_nomination {
	ICE_NOMINATION_REGULAR = 0,
	ICE_NOMINATION_AGGRESSIVE
};

/** ICE Candidate type */
enum ice_cand_type {
	ICE_CAND_TYPE_HOST,   /**< Host candidate             */
	ICE_CAND_TYPE_SRFLX,  /**< Server Reflexive candidate */
	ICE_CAND_TYPE_PRFLX,  /**< Peer Reflexive candidate   */
	ICE_CAND_TYPE_RELAY   /**< Relayed candidate          */
};

/** ICE TCP protocol type */
enum ice_tcptype {
	ICE_TCP_ACTIVE,   /**< Active TCP client                   */
	ICE_TCP_PASSIVE,  /**< Passive TCP server                  */
	ICE_TCP_SO        /**< Simultaneous-open TCP client/server */
};

struct ice;
struct icem;

/** ICE Configuration */
struct ice_conf {
	enum ice_nomination nom;  /**< Nomination algorithm        */
	uint32_t rto;             /**< STUN Retransmission TimeOut */
	uint32_t rc;              /**< STUN Retransmission Count   */
	bool debug;               /**< Enable ICE debugging        */
};

typedef void (ice_gather_h)(int err, uint16_t scode, const char *reason,
			    void *arg);
typedef void (ice_connchk_h)(int err, bool update, void *arg);


/* ICE Session */
int  ice_alloc(struct ice **icep, enum ice_mode mode, bool offerer);
struct ice_conf *ice_conf(struct ice *ice);
void ice_set_conf(struct ice *ice, const struct ice_conf *conf);
void ice_set_offerer(struct ice *ice, bool offerer);
int  ice_sdp_decode(struct ice *ice, const char *name, const char *value);
int  ice_conncheck_start(struct ice *ice);
int  ice_debug(struct re_printf *pf, const struct ice *ice);
struct list *ice_medialist(const struct ice *ice);
const char *ice_ufrag(const struct ice *ice);
const char *ice_pwd(const struct ice *ice);


/* ICE Media */
int  icem_alloc(struct icem **icemp, struct ice *ice, int proto, int layer,
		ice_gather_h *gh, ice_connchk_h *chkh, void *arg);
void icem_set_name(struct icem *icem, const char *name);
int  icem_comp_add(struct icem *icem, unsigned compid, void *sock);
int  icem_cand_add(struct icem *icem, unsigned compid, uint16_t lprio,
		   const char *ifname, const struct sa *addr);
int  icem_gather_srflx(struct icem *icem, const struct sa *stun_srv);
int  icem_gather_relay(struct icem *icem, const struct sa *stun_srv,
		       const char *username, const char *password);
int  icem_lite_set_default_candidates(struct icem *icem);
bool icem_verify_support(struct icem *icem, unsigned compid,
			 const struct sa *raddr);
int  icem_conncheck_start(struct icem *icem);
void icem_conncheck_stop(struct icem *icem, int err);
int  icem_add_chan(struct icem *icem, unsigned compid, const struct sa *raddr);
bool icem_mismatch(const struct icem *icem);
void icem_update(struct icem *icem);
int  icem_sdp_decode(struct icem *icem, const char *name, const char *value);
int  icem_debug(struct re_printf *pf, const struct icem *icem);
struct list *icem_lcandl(const struct icem *icem);
struct list *icem_rcandl(const struct icem *icem);
struct list *icem_checkl(const struct icem *icem);
struct list *icem_validl(const struct icem *icem);
const struct sa *icem_cand_default(struct icem *icem, unsigned compid);
const struct sa *icem_selected_laddr(const struct icem *icem, unsigned compid);


struct ice_cand;
bool ice_remotecands_avail(const struct icem *icem);
int  ice_cand_encode(struct re_printf *pf, const struct ice_cand *cand);
int  ice_remotecands_encode(struct re_printf *pf, const struct icem *icem);


extern const char ice_attr_cand[];
extern const char ice_attr_lite[];
extern const char ice_attr_mismatch[];
extern const char ice_attr_pwd[];
extern const char ice_attr_remote_cand[];
extern const char ice_attr_ufrag[];


const char        *ice_cand_type2name(enum ice_cand_type type);
enum ice_cand_type ice_cand_name2type(const char *name);


uint32_t ice_cand_calc_prio(enum ice_cand_type type, uint16_t local,
			    unsigned compid);


/** Defines an SDP candidate attribute */
struct ice_cand_attr {
	char foundation[32];      /**< Foundation string                    */
	unsigned compid;          /**< Component ID (1-256)                 */
	int proto;                /**< Transport protocol                   */
	uint32_t prio;            /**< Priority of this candidate           */
	struct sa addr;           /**< Transport address                    */
	enum ice_cand_type type;  /**< Candidate type                       */
	struct sa rel_addr;       /**< Related transport address (optional) */
	enum ice_tcptype tcptype; /**< TCP candidate type (TCP-only)        */
};

int ice_cand_attr_encode(struct re_printf *pf,
			 const struct ice_cand_attr *cand);
int ice_cand_attr_decode(struct ice_cand_attr *cand, const char *val);
