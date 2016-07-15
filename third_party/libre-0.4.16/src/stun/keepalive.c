/**
 * @file stun/keepalive.c  STUN usage for NAT Keepalives
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_stun.h>


#define DEBUG_MODULE "keepalive"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/** Defines a STUN Keepalive session */
struct stun_keepalive {
	struct stun_ctrans *ct;   /**< STUN client transaction              */
	struct stun *stun;        /**< STUN instance                        */
	struct udp_helper *uh;
	int proto;
	void *sock;
	struct sa dst;
	struct tmr tmr;           /**< Refresh timer                        */
	uint32_t interval;        /**< Refresh interval in seconds          */
	stun_mapped_addr_h *mah;  /**< Mapped address handler               */
	void *arg;                /**< Handler argument                     */
	struct sa map;            /**< Mapped IP address and port           */
	struct sa xormap;         /**< XOR-Mapped IP address and port       */
	struct sa curmap;         /**< Currently mapped IP address and port */
};

static void timeout(void *arg);


static void keepalive_destructor(void *data)
{
	struct stun_keepalive *ska = data;

	tmr_cancel(&ska->tmr);

	mem_deref(ska->ct);
	mem_deref(ska->uh);
	mem_deref(ska->sock);
	mem_deref(ska->stun);
}


static void call_handler(struct stun_keepalive *ska, int err,
			 const struct sa *map)
{
	if (ska->mah)
		ska->mah(err, map, ska->arg);
}


static void stun_response_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct stun_keepalive *ska = arg;
	struct stun_attr *attr;
	(void)reason;

	/* Restart timer */
	if (ska->interval > 0)
		tmr_start(&ska->tmr, ska->interval*1000, timeout, ska);

	if (err || scode) {
		/* Clear current mapped addr to force new notification */
		sa_set_in(&ska->curmap, 0, 0);

		goto out;
	}

	attr = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
	if (!attr)
		attr = stun_msg_attr(msg, STUN_ATTR_MAPPED_ADDR);

	if (!attr) {
		err = ENOENT;
		goto out;
	}

	if (!sa_cmp(&ska->curmap, &attr->v.sa, SA_ALL)) {
		ska->curmap = attr->v.sa;
		call_handler(ska, 0, &ska->curmap);
	}

 out:
	if (err)
		call_handler(ska, err, NULL);
}


static void timeout(void *arg)
{
	struct stun_keepalive *ska = arg;
	int err;

	if (ska->ct)
		ska->ct = mem_deref(ska->ct);

	err = stun_request(&ska->ct, ska->stun, ska->proto, ska->sock,
			   &ska->dst, 0, STUN_METHOD_BINDING, NULL, 0, false,
			   stun_response_handler, ska, 1,
			   STUN_ATTR_SOFTWARE, stun_software);
	if (0 == err)
		return;

	/* Restart timer */
	if (ska->interval > 0)
		tmr_start(&ska->tmr, ska->interval*1000, timeout, ska);

	/* Error */
	call_handler(ska, err, NULL);
}


static bool udp_recv_handler(struct sa *src, struct mbuf *mb, void *arg)
{
	struct stun_keepalive *ska = arg;
	struct stun_unknown_attr ua;
	struct stun_msg *msg;
	size_t pos = mb->pos;
	bool hdld;

	if (!sa_cmp(&ska->dst, src, SA_ALL))
		return false;

	if (stun_msg_decode(&msg, mb, &ua))
		return false;

	if (stun_msg_method(msg) != STUN_METHOD_BINDING) {
		hdld = false;
		mb->pos = pos;
		goto out;
	}

	switch (stun_msg_class(msg)) {

	case STUN_CLASS_ERROR_RESP:
	case STUN_CLASS_SUCCESS_RESP:
		(void)stun_ctrans_recv(ska->stun, msg, &ua);
		hdld = true;
		break;

	default:
		hdld = false;
		mb->pos = pos;
		break;
	}

 out:
	mem_deref(msg);

	return hdld;
}


/**
 * Allocate a new STUN keepalive session
 *
 * @param skap  Pointer to keepalive object
 * @param proto Transport protocol
 * @param sock  Socket
 * @param layer Protocol layer
 * @param dst   Destination address
 * @param conf  Configuration
 * @param mah   Mapped address handler
 * @param arg   Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int stun_keepalive_alloc(struct stun_keepalive **skap,
			 int proto, void *sock, int layer,
			 const struct sa *dst, const struct stun_conf *conf,
			 stun_mapped_addr_h *mah, void *arg)
{
	struct stun_keepalive *ska;
	int err;

	if (!skap)
		return EINVAL;

	ska = mem_zalloc(sizeof(*ska), keepalive_destructor);
	if (!ska)
		return ENOMEM;

	err = stun_alloc(&ska->stun, conf, NULL, NULL);
	if (err)
		goto out;

	tmr_init(&ska->tmr);

	ska->proto = proto;
	ska->sock = mem_ref(sock);
	ska->mah = mah;
	ska->arg = arg;

	if (dst)
		ska->dst = *dst;

	switch (proto) {

	case IPPROTO_UDP:
		err = udp_register_helper(&ska->uh, sock, layer,
					  NULL, udp_recv_handler, ska);
		break;

	default:
		err = 0;
		break;
	}

 out:
	if (err)
		mem_deref(ska);
	else
		*skap = ska;

	return err;
}


/**
 * Enable or disable keepalive timer
 *
 * @param ska      Keepalive object
 * @param interval Interval in seconds (0 to disable)
 *
 * @return 0 if success, otherwise errorcode
 */
void stun_keepalive_enable(struct stun_keepalive *ska, uint32_t interval)
{
	if (!ska)
		return;

	ska->interval = interval;

	tmr_cancel(&ska->tmr);
	if (interval > 0)
		tmr_start(&ska->tmr, 1, timeout, ska);
}
