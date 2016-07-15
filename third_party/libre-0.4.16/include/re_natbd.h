/**
 * @file re_natbd.h  NAT Behavior Discovery Using STUN (RFC 5780)
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** NAT Mapping/Filtering types - See RFC 4787 for definitions */
enum nat_type {
	NAT_TYPE_UNKNOWN       = 0,  /**< Unknown type               */
	NAT_TYPE_ENDP_INDEP    = 1,  /**< Endpoint-Independent       */
	NAT_TYPE_ADDR_DEP      = 2,  /**< Address-Dependent          */
	NAT_TYPE_ADDR_PORT_DEP = 3   /**< Address and Port-Dependent */
};


/* Strings */
const char *nat_type_str(enum nat_type type);


/*
 * Diagnosing NAT Hairpinning
 */
struct nat_hairpinning;
/**
 * Defines the NAT Hairpinning handler
 */
typedef void (nat_hairpinning_h)(int err, bool supported, void *arg);

int nat_hairpinning_alloc(struct nat_hairpinning **nhp,
			  const struct sa *srv, int proto,
			  const struct stun_conf *conf,
			  nat_hairpinning_h *hph, void *arg);
int nat_hairpinning_start(struct nat_hairpinning *nh);


/*
 * Determining NAT Mapping Behavior
 */
struct nat_mapping;

/**
 * Defines the NAT Mapping handler
 *
 * @param err  Errorcode
 * @param type NAT Mapping type
 * @param arg  Handler argument
 */
typedef void (nat_mapping_h)(int err, enum nat_type map, void *arg);

int nat_mapping_alloc(struct nat_mapping **nmp, const struct sa *laddr,
		      const struct sa *srv, int proto,
		      const struct stun_conf *conf,
		      nat_mapping_h *mh, void *arg);
int nat_mapping_start(struct nat_mapping *nm);


/*
 * Determining NAT Filtering Behavior
 */
struct nat_filtering;

/**
 * Defines the NAT Filtering handler
 *
 * @param err  Errorcode
 * @param type NAT Filtering type
 * @param arg  Handler argument
 */
typedef void (nat_filtering_h)(int err, enum nat_type filt, void *arg);

int nat_filtering_alloc(struct nat_filtering **nfp, const struct sa *srv,
			const struct stun_conf *conf,
			nat_filtering_h *fh, void *arg);
int nat_filtering_start(struct nat_filtering *nf);


/*
 * Binding Lifetime Discovery
 */

struct nat_lifetime;

/** Defines the NAT lifetime interval */
struct nat_lifetime_interval {
	uint32_t min;  /**< Minimum lifetime interval in [seconds] */
	uint32_t cur;  /**< Current lifetime interval in [seconds] */
	uint32_t max;  /**< Maximum lifetime interval in [seconds] */
};


/**
 * Defines the NAT Lifetime handler
 *
 * @param err Errorcode
 * @param i   NAT Lifetime intervals
 * @param arg Handler argument
 */
typedef void (nat_lifetime_h)(int err, const struct nat_lifetime_interval *i,
			      void *arg);

int  nat_lifetime_alloc(struct nat_lifetime **nlp, const struct sa *srv,
			uint32_t interval, const struct stun_conf *conf,
			nat_lifetime_h *lh, void *arg);
int  nat_lifetime_start(struct nat_lifetime *nl);


/*
 * Detecting Generic ALGs
 */
struct nat_genalg;

/**
 * Defines the NAT Generic ALG handler
 *
 * @param err     Errorcode
 * @param errcode STUN Error code (if set)
 * @param status  Generic ALG status (-1=not detected, 1=detected)
 * @param map     Mapped network address
 * @param arg     Handler argument
 */
typedef void (nat_genalg_h)(int err, uint16_t scode, const char *reason,
			    int status, const struct sa *map, void *arg);

int nat_genalg_alloc(struct nat_genalg **ngp, const struct sa *srv, int proto,
		     const struct stun_conf *conf,
		     nat_genalg_h *gh, void *arg);
int nat_genalg_start(struct nat_genalg *ng);
