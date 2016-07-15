/**
 * @file ice.h  Internal Interface to ICE
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifndef RELEASE
#define ICE_TRACE 1    /**< Trace connectivity checks */
#endif


enum role {
	ROLE_UNKNOWN = 0,
	ROLE_CONTROLLING,
	ROLE_CONTROLLED
};

enum ice_checkl_state {
	ICE_CHECKLIST_NULL = -1,
	ICE_CHECKLIST_RUNNING,
	ICE_CHECKLIST_COMPLETED,
	ICE_CHECKLIST_FAILED
};

/** Candidate pair states */
enum ice_candpair_state {
	ICE_CANDPAIR_FROZEN = 0, /**< Frozen state (default)                 */
	ICE_CANDPAIR_WAITING,    /**< Waiting to become highest on list      */
	ICE_CANDPAIR_INPROGRESS, /**< In-Progress state;transac. in progress */
	ICE_CANDPAIR_SUCCEEDED,  /**< Succeeded state; successful result     */
	ICE_CANDPAIR_FAILED      /**< Failed state; check failed             */
};

enum ice_transp {
	ICE_TRANSP_NONE = -1,
	ICE_TRANSP_UDP  = IPPROTO_UDP
};

/** ICE protocol values */
enum {
	ICE_DEFAULT_Tr          =  15, /**< Keepalive interval [s]          */
	ICE_DEFAULT_Ta_RTP      =  20, /**< Pacing interval RTP [ms]        */
	ICE_DEFAULT_Ta_NON_RTP  = 500, /**< Pacing interval [ms]            */
	ICE_DEFAULT_RTO_RTP     = 100, /**< Retransmission TimeOut RTP [ms] */
	ICE_DEFAULT_RTO_NONRTP  = 500, /**< Retransmission TimeOut [ms]     */
	ICE_DEFAULT_RC          =   7  /**< Retransmission count            */
};


/** Defines an ICE session */
struct ice {
	enum ice_mode lmode;          /**< Local mode                       */
	enum ice_mode rmode;          /**< Remote mode                      */
	enum role lrole;              /**< Local role                       */
	char lufrag[5];               /**< Local Username fragment          */
	char lpwd[23];                /**< Local Password                   */
	struct list ml;               /**< Media list (struct icem)         */
	uint64_t tiebrk;              /**< Tie-break value for roleconflict */
	struct ice_conf conf;         /**< ICE Configuration                */
	struct stun *stun;            /**< STUN Transport                   */
};

/** Defines a media-stream component */
struct icem_comp {
	struct le le;                /**< Linked-list element               */
	struct icem *icem;           /**< Parent ICE media                  */
	struct ice_cand *def_lcand;  /**< Default local candidate           */
	struct ice_cand *def_rcand;  /**< Default remote candidate          */
	struct ice_candpair *cp_sel; /**< Selected candidate-pair           */
	struct udp_helper *uh;       /**< UDP helper                        */
	void *sock;                  /**< Transport socket                  */
	uint16_t lport;              /**< Local port number                 */
	unsigned id;                 /**< Component ID                      */
	bool concluded;              /**< Concluded flag                    */
	struct turnc *turnc;         /**< TURN Client                       */
	struct stun_ctrans *ct_gath; /**< STUN Transaction for gathering    */
	struct tmr tmr_ka;           /**< Keep-alive timer                  */
};

/** Defines an ICE media-stream */
struct icem {
	struct le le;                /**< Linked-list element                */
	struct ice *ice;             /**< Pointer to parent ICE-session      */
	struct sa stun_srv;          /**< STUN Server IP address and port    */
	int nstun;                   /**< Number of pending STUN candidates  */
	struct list lcandl;          /**< List of local candidates           */
	struct list rcandl;          /**< List of remote candidates          */
	struct list checkl;          /**< Check List of cand pairs (sorted)  */
	struct list validl;          /**< Valid List of cand pairs (sorted)  */
	bool mismatch;               /**< ICE mismatch flag                  */
	struct tmr tmr_pace;         /**< Timer for pacing STUN requests     */
	int proto;                   /**< Transport protocol                 */
	int layer;                   /**< Protocol layer                     */
	enum ice_checkl_state state; /**< State of the checklist             */
	struct list compl;           /**< ICE media components               */
	char *rufrag;                /**< Remote Username fragment           */
	char *rpwd;                  /**< Remote Password                    */
	ice_gather_h *gh;            /**< Gather handler                     */
	ice_connchk_h *chkh;         /**< Connectivity check handler         */
	void *arg;                   /**< Handler argument                   */
	char name[32];               /**< Name of the media stream           */
};

/** Defines a candidate */
struct ice_cand {
	struct le le;                /**< List element                       */
	enum ice_cand_type type;     /**< Candidate type                     */
	uint32_t prio;               /**< Priority of this candidate         */
	char *foundation;            /**< Foundation                         */
	unsigned compid;             /**< Component ID (1-256)               */
	struct sa rel;               /**< Related IP address and port number */
	struct sa addr;              /**< Transport address                  */
	enum ice_transp transp;      /**< Transport protocol                 */

	/* extra for local */
	struct ice_cand *base;       /**< Links to base candidate, if any    */
	char *ifname;                /**< Network interface, for diagnostics */
};

/** Defines a candidate pair */
struct ice_candpair {
	struct le le;                /**< List element                       */
	struct icem *icem;           /**< Pointer to parent ICE media        */
	struct icem_comp *comp;      /**< Pointer to media-stream component  */
	struct ice_cand *lcand;      /**< Local candidate                    */
	struct ice_cand *rcand;      /**< Remote candidate                   */
	bool def;                    /**< Default flag                       */
	bool valid;                  /**< Valid flag                         */
	bool nominated;              /**< Nominated flag                     */
	enum ice_candpair_state state;/**< Candidate pair state              */
	uint64_t pprio;              /**< Pair priority                      */
	struct stun_ctrans *ct_conn; /**< STUN Transaction for conncheck     */
	int err;                     /**< Saved error code, if failed        */
	uint16_t scode;              /**< Saved STUN code, if failed         */
};


/* cand */
int icem_lcand_add_base(struct icem *icem, unsigned compid, uint16_t lprio,
			const char *ifname, enum ice_transp transp,
			const struct sa *addr);
int icem_lcand_add(struct icem *icem, struct ice_cand *base,
		   enum ice_cand_type type,
		   const struct sa *addr);
int icem_rcand_add(struct icem *icem, enum ice_cand_type type, unsigned compid,
		   uint32_t prio, const struct sa *addr,
		   const struct sa *rel_addr, const struct pl *foundation);
int icem_rcand_add_prflx(struct ice_cand **rcp, struct icem *icem,
			 unsigned compid, uint32_t prio,
			 const struct sa *addr);
struct ice_cand *icem_cand_find(const struct list *lst, unsigned compid,
				const struct sa *addr);
struct ice_cand *icem_lcand_find_checklist(const struct icem *icem,
					   unsigned compid);
int icem_cands_debug(struct re_printf *pf, const struct list *lst);
int icem_cand_print(struct re_printf *pf, const struct ice_cand *cand);


/* candpair */
int  icem_candpair_alloc(struct ice_candpair **cpp, struct icem *icem,
			 struct ice_cand *lcand, struct ice_cand *rcand);
int  icem_candpair_clone(struct ice_candpair **cpp, struct ice_candpair *cp0,
			 struct ice_cand *lcand, struct ice_cand *rcand);
void icem_candpair_prio_order(struct list *lst);
void icem_candpair_cancel(struct ice_candpair *cp);
void icem_candpair_make_valid(struct ice_candpair *cp);
void icem_candpair_failed(struct ice_candpair *cp, int err, uint16_t scode);
void icem_candpair_set_state(struct ice_candpair *cp,
			     enum ice_candpair_state state);
void icem_candpairs_flush(struct list *lst, enum ice_cand_type type,
			  unsigned compid);
bool icem_candpair_iscompleted(const struct ice_candpair *cp);
bool icem_candpair_cmp(const struct ice_candpair *cp1,
		       const struct ice_candpair *cp2);
bool icem_candpair_cmp_fnd(const struct ice_candpair *cp1,
			   const struct ice_candpair *cp2);
struct ice_candpair *icem_candpair_find(const struct list *lst,
				    const struct ice_cand *lcand,
				    const struct ice_cand *rcand);
struct ice_candpair *icem_candpair_find_st(const struct list *lst,
					   unsigned compid,
					   enum ice_candpair_state state);
struct ice_candpair *icem_candpair_find_compid(const struct list *lst,
					   unsigned compid);
struct ice_candpair *icem_candpair_find_rcand(struct icem *icem,
					  const struct ice_cand *rcand);
int  icem_candpair_debug(struct re_printf *pf, const struct ice_candpair *cp);
int  icem_candpairs_debug(struct re_printf *pf, const struct list *list);


/* stun server */
int icem_stund_recv(struct icem_comp *comp, const struct sa *src,
		    struct stun_msg *req, size_t presz);


/* ICE media */
void icem_cand_redund_elim(struct icem *icem);
void icem_printf(struct icem *icem, const char *fmt, ...);


/* Checklist */
int  icem_checklist_form(struct icem *icem);
void icem_checklist_update(struct icem *icem);


/* component */
int  icem_comp_alloc(struct icem_comp **cp, struct icem *icem, int id,
		     void *sock);
int  icem_comp_set_default_cand(struct icem_comp *comp);
void icem_comp_set_default_rcand(struct icem_comp *comp,
				 struct ice_cand *rcand);
void icem_comp_set_selected(struct icem_comp *comp, struct ice_candpair *cp);
struct icem_comp *icem_comp_find(const struct icem *icem, unsigned compid);
void icem_comp_keepalive(struct icem_comp *comp, bool enable);
void icecomp_printf(struct icem_comp *comp, const char *fmt, ...);
int  icecomp_debug(struct re_printf *pf, const struct icem_comp *comp);


/* conncheck */
void icem_conncheck_schedule_check(struct icem *icem);
void icem_conncheck_continue(struct icem *icem);
int  icem_conncheck_send(struct ice_candpair *cp, bool use_cand, bool trigged);


/* icestr */
const char    *ice_mode2name(enum ice_mode mode);
const char    *ice_role2name(enum role role);
const char    *ice_candpair_state2name(enum ice_candpair_state st);
const char    *ice_checkl_state2name(enum ice_checkl_state cst);


/* util */
typedef void * (list_unique_h)(struct le *le1, struct le *le2);

uint64_t ice_calc_pair_prio(uint32_t g, uint32_t d);
void ice_switch_local_role(struct ice *ice);
uint32_t ice_list_unique(struct list *list, list_unique_h *uh);
