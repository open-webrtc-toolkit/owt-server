/**
 * @file re_dns.h  Interface to DNS module
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	DNS_PORT = 53,
	DNS_HEADER_SIZE = 12
};


/** DNS Opcodes */
enum {
	DNS_OPCODE_QUERY  = 0,
	DNS_OPCODE_IQUERY = 1,
	DNS_OPCODE_STATUS = 2,
	DNS_OPCODE_NOTIFY = 4
};


/** DNS Response codes */
enum {
	DNS_RCODE_OK       = 0,
	DNS_RCODE_FMT_ERR  = 1,
	DNS_RCODE_SRV_FAIL = 2,
	DNS_RCODE_NAME_ERR = 3,
	DNS_RCODE_NOT_IMPL = 4,
	DNS_RCODE_REFUSED  = 5,
	DNS_RCODE_NOT_AUTH = 9
};


/** DNS Resource Record types */
enum {
	DNS_TYPE_A     = 0x0001,
	DNS_TYPE_NS    = 0x0002,
	DNS_TYPE_CNAME = 0x0005,
	DNS_TYPE_SOA   = 0x0006,
	DNS_TYPE_PTR   = 0x000c,
	DNS_TYPE_MX    = 0x000f,
	DNS_TYPE_AAAA  = 0x001c,
	DNS_TYPE_SRV   = 0x0021,
	DNS_TYPE_NAPTR = 0x0023,
	DNS_QTYPE_IXFR = 0x00fb,
	DNS_QTYPE_AXFR = 0x00fc,
	DNS_QTYPE_ANY  = 0x00ff
};


/** DNS Classes */
enum {
	DNS_CLASS_IN   = 0x0001,
	DNS_QCLASS_ANY = 0x00ff
};


/** Defines a DNS Header */
struct dnshdr {
	uint16_t id;
	bool qr;
	uint8_t opcode;
	bool aa;
	bool tc;
	bool rd;
	bool ra;
	uint8_t z;
	uint8_t rcode;
	uint16_t nq;
	uint16_t nans;
	uint16_t nauth;
	uint16_t nadd;
};


/** Defines a DNS Resource Record (RR) */
struct dnsrr {
	struct le le;
	struct le le_priv;
	char *name;
	uint16_t type;
	uint16_t dnsclass;
	int64_t ttl;
	uint16_t rdlen;
	union {
		struct {
			uint32_t addr;
		} a;
		struct {
			char *nsdname;
		} ns;
		struct {
			char *cname;
		} cname;
		struct {
			char *mname;
			char *rname;
			uint32_t serial;
			uint32_t refresh;
			uint32_t retry;
			uint32_t expire;
			uint32_t ttlmin;
		} soa;
		struct {
			char *ptrdname;
		} ptr;
		struct {
			uint16_t pref;
			char *exchange;
		} mx;
		struct {
			uint8_t addr[16];
		} aaaa;
		struct {
			uint16_t pri;
			uint16_t weight;
			uint16_t port;
			char *target;
		} srv;
		struct {
			uint16_t order;
			uint16_t pref;
			char *flags;
			char *services;
			char *regexp;
			char *replace;
		} naptr;
	} rdata;
};

struct hash;

/**
 * Defines the DNS Query handler
 *
 * @param err   0 if success, otherwise errorcode
 * @param hdr   DNS Header
 * @param ansl  List of Answer records
 * @param authl List of Authoritive records
 * @param addl  List of Additional records
 * @param arg   Handler argument
 */
typedef void(dns_query_h)(int err, const struct dnshdr *hdr,
			  struct list *ansl, struct list *authl,
			  struct list *addl, void *arg);

/**
 * Defines the DNS Resource Record list handler
 *
 * @param rr  DNS Resource Record
 * @param arg Handler argument
 *
 * @return True to stop traversing, False to continue
 */
typedef bool(dns_rrlist_h)(struct dnsrr *rr, void *arg);

int  dns_hdr_encode(struct mbuf *mb, const struct dnshdr *hdr);
int  dns_hdr_decode(struct mbuf *mb, struct dnshdr *hdr);
const char *dns_hdr_opcodename(uint8_t opcode);
const char *dns_hdr_rcodename(uint8_t rcode);
struct dnsrr *dns_rr_alloc(void);
int  dns_rr_encode(struct mbuf *mb, const struct dnsrr *rr, int64_t ttl_offs,
		   struct hash *ht_dname, size_t start);
int  dns_rr_decode(struct mbuf *mb, struct dnsrr **rr, size_t start);
bool dns_rr_cmp(const struct dnsrr *rr1, const struct dnsrr *rr2, bool rdata);
const char *dns_rr_typename(uint16_t type);
const char *dns_rr_classname(uint16_t dnsclass);
int  dns_rr_print(struct re_printf *pf, const struct dnsrr *rr);
int  dns_dname_encode(struct mbuf *mb, const char *name,
		      struct hash *ht_dname, size_t start, bool comp);
int  dns_dname_decode(struct mbuf *mb, char **name, size_t start);
int  dns_cstr_encode(struct mbuf *mb, const char *str);
int  dns_cstr_decode(struct mbuf *mb, char **str);
void dns_rrlist_sort(struct list *rrl, uint16_t type);
struct dnsrr *dns_rrlist_apply(struct list *rrl, const char *name,
			       uint16_t type, uint16_t dnsclass,
			       bool recurse, dns_rrlist_h *rrlh, void *arg);
struct dnsrr *dns_rrlist_apply2(struct list *rrl, const char *name,
				uint16_t type1, uint16_t type2,
				uint16_t dnsclass, bool recurse,
				dns_rrlist_h *rrlh, void *arg);
struct dnsrr *dns_rrlist_find(struct list *rrl, const char *name,
			      uint16_t type, uint16_t dnsclass, bool recurse);


/* DNS Client */
struct sa;
struct dnsc;
struct dns_query;

/** DNS Client configuration */
struct dnsc_conf {
	uint32_t query_hash_size;
	uint32_t tcp_hash_size;
	uint32_t conn_timeout;  /* in [ms] */
	uint32_t idle_timeout;  /* in [ms] */
};

int  dnsc_alloc(struct dnsc **dcpp, const struct dnsc_conf *conf,
		const struct sa *srvv, uint32_t srvc);
int  dnsc_srv_set(struct dnsc *dnsc, const struct sa *srvv, uint32_t srvc);
int  dnsc_query(struct dns_query **qp, struct dnsc *dnsc, const char *name,
		uint16_t type, uint16_t dnsclass,
		bool rd, dns_query_h *qh, void *arg);
int  dnsc_query_srv(struct dns_query **qp, struct dnsc *dnsc, const char *name,
		    uint16_t type, uint16_t dnsclass, int proto,
		    const struct sa *srvv, const uint32_t *srvc,
		    bool rd, dns_query_h *qh, void *arg);
int  dnsc_notify(struct dns_query **qp, struct dnsc *dnsc, const char *name,
		 uint16_t type, uint16_t dnsclass, const struct dnsrr *ans_rr,
		 int proto, const struct sa *srvv, const uint32_t *srvc,
		 dns_query_h *qh, void *arg);


/* DNS System functions */
int dns_srv_get(char *domain, size_t dsize, struct sa *srvv, uint32_t *n);
