/**
 * @file lifetime.c  NAT Binding Lifetime Discovery
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_stun.h>
#include <re_natbd.h>


#define DEBUG_MODULE "natbd_lifetime"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/* Binding Lifetime Discovery

   STUN can also be used to probe the lifetimes of the bindings created
   by the NAT.  For many NAT devices, an absolute refresh interval
   cannot be determined; bindings might be closed quicker under heavy
   load or might not behave as the tests suggest.  For this reason
   applications that require reliable bindings must send keep-alives as
   frequently as required by all NAT devices that will be encountered.
 */


/** Defines a NAT Binding Lifetime Discovery session */
struct nat_lifetime {
	struct stun *stun;                      /**< STUN Client            */
	struct stun_ctrans *ctx;                /**< STUN Transaction 1     */
	struct stun_ctrans *cty;                /**< STUN Transaction 2     */
	struct udp_sock *us_x;                  /**< First UDP socket       */
	struct udp_sock *us_y;                  /**< Second UDP socket      */
	struct sa srv;                          /**< Server IP-address/port */
	struct sa map;                          /**< Mapped IP address/port */
	struct tmr tmr;                         /**< Refresh timer          */
	bool probing;                           /**< Probing flag           */
	struct nat_lifetime_interval interval;  /**< Lifetime intervals     */
	nat_lifetime_h *lh;                     /**< Result handler         */
	void *arg;                              /**< Handler argument       */
};


static void timeout(void *arg);
static void binding_ok(struct nat_lifetime *nl);
static void binding_expired(struct nat_lifetime *nl);


/*
 * X socket
 */


static void msg_recv(struct stun *stun, const struct sa *src,
		     struct mbuf *mb)
{
	int err;
	(void)src;

	err = stun_recv(stun, mb);
	if (err && ENOENT != err) {
		DEBUG_WARNING("msg_recv: stunc_recv(): (%m)\n", err);
	}
}


static void udp_recv_handler_x(const struct sa *src, struct mbuf *mb,
			       void *arg)
{
	struct nat_lifetime *nl = arg;

	/* Forward response to socket */
	msg_recv(nl->stun, src, mb);
}


static void stun_response_handler_x(int err, uint16_t scode,
				    const char *reason,
				    const struct stun_msg *msg, void *arg)
{
	struct nat_lifetime *nl = arg;
	struct stun_attr *attr;

	(void)reason;

	if (err) {
		DEBUG_WARNING("stun_response_handler X: %m\n", err);
		goto out;
	}

	switch (scode) {

	case 0:
		attr = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
		if (!attr) {
			err = EPROTO;
			break;
		}

		nl->map = attr->v.xor_mapped_addr;

		DEBUG_INFO("Starting timer of %d seconds...[zzz]...\n",
			   nl->interval.cur);

		tmr_start(&nl->tmr, nl->interval.cur*1000, timeout, nl);
		return;

	default:
		err = EPROTO;
		break;
	}

 out:
	nl->lh(err, &nl->interval, nl->arg);
	binding_expired(nl);
}


/*
 * Y socket
 */


static void udp_recv_handler_y(const struct sa *src, struct mbuf *mb,
			       void *arg)
{
	struct nat_lifetime *nl = arg;

	(void)src;
	(void)mb;

	if (!nl->probing) {
		DEBUG_WARNING("Y: hmm, not probing?\n");
	}

	binding_expired(nl);
}


static void stun_response_handler_y(int err, uint16_t scode,
				    const char *reason,
				    const struct stun_msg *msg, void *arg)
{
	struct nat_lifetime *nl = arg;
	(void)reason;
	(void)msg;

	if (err) {
		binding_expired(nl);
		return;
	}

	switch (scode) {

	case 0:
		binding_ok(nl);
		break;

	default:
		nl->lh(EBADMSG, &nl->interval, nl->arg);
		break;
	}
}


/*
 * Common
 */


static int start_test(struct nat_lifetime *nl)
{
	nl->probing = false;

	tmr_cancel(&nl->tmr);

	nl->ctx = mem_deref(nl->ctx);
	return stun_request(&nl->ctx, nl->stun, IPPROTO_UDP, nl->us_x,
			    &nl->srv, 0, STUN_METHOD_BINDING, NULL, 0, false,
			    stun_response_handler_x, nl, 1,
			    STUN_ATTR_SOFTWARE, stun_software);
}


static void timeout(void *arg)
{
	struct nat_lifetime *nl = arg;
	const uint16_t rp = sa_port(&nl->map);
	int err;

	nl->probing = true;

	nl->cty = mem_deref(nl->cty);
	err = stun_request(&nl->cty, nl->stun, IPPROTO_UDP, nl->us_y,
			   &nl->srv, 0, STUN_METHOD_BINDING, NULL, 0, false,
			   stun_response_handler_y, nl, 2,
			   STUN_ATTR_RESP_PORT, &rp,
			   STUN_ATTR_SOFTWARE, stun_software);
	if (err)
		goto out;

	return;

 out:
	DEBUG_WARNING("timeout: (%m)\n", err);

	nl->lh(err, &nl->interval, nl->arg);

	(void)start_test(nl);
}


/* Binding OK - recalculate current interval */
static void binding_ok(struct nat_lifetime *nl)
{
	nl->interval.min = max(1, nl->interval.cur);

	if (nl->interval.max > 0)
		nl->interval.cur = (nl->interval.min + nl->interval.max) / 2;
	else
		nl->interval.cur *= 2;

	nl->lh(0, &nl->interval, nl->arg);

	(void)start_test(nl);
}


/* Request timed out - recalculate current interval */
static void binding_expired(struct nat_lifetime *nl)
{
	nl->interval.max = nl->interval.cur;

	nl->interval.cur = (nl->interval.min + nl->interval.max) / 2;

	nl->lh(0, &nl->interval, nl->arg);

	(void)start_test(nl);
}


static void lifetime_destructor(void *data)
{
	struct nat_lifetime *nl = data;

	tmr_cancel(&nl->tmr);

	mem_deref(nl->ctx);
	mem_deref(nl->cty);
	mem_deref(nl->us_x);
	mem_deref(nl->us_y);
	mem_deref(nl->stun);
}


/**
 * Allocate a new NAT Lifetime discovery session
 *
 * @param nlp       Pointer to allocated NAT Lifetime object
 * @param srv       STUN Server IP address and port number
 * @param interval  Initial interval in [seconds]
 * @param conf      STUN configuration (Optional)
 * @param lh        Lifetime handler - called for each probe
 * @param arg       Handler argument
 *
 * @return 0 if success, errorcode if failure
 */
int nat_lifetime_alloc(struct nat_lifetime **nlp, const struct sa *srv,
		       uint32_t interval, const struct stun_conf *conf,
		       nat_lifetime_h *lh, void *arg)
{
	struct nat_lifetime *nl;
	int err;

	if (!nlp || !srv || !interval || !lh)
		return EINVAL;

	nl = mem_zalloc(sizeof(*nl), lifetime_destructor);
	if (!nl)
		return ENOMEM;

	tmr_init(&nl->tmr);

	err = stun_alloc(&nl->stun, conf, NULL, NULL);
	if (err)
		goto out;

	err = udp_listen(&nl->us_x, NULL, udp_recv_handler_x, nl);
	if (err)
		goto out;

	err = udp_listen(&nl->us_y, NULL, udp_recv_handler_y, nl);
	if (err)
		goto out;

	sa_cpy(&nl->srv, srv);
	nl->interval.min = 1;
	nl->interval.cur = interval;
	nl->lh  = lh;
	nl->arg = arg;

 out:
	if (err)
		mem_deref(nl);
	else
		*nlp = nl;

	return err;
}


/**
 * Start a new NAT Lifetime discovery session
 *
 * @param nl NAT Lifetime object
 *
 * @return 0 if success, errorcode if failure
 */
int nat_lifetime_start(struct nat_lifetime *nl)
{
	if (!nl)
		return EINVAL;

	return start_test(nl);
}
