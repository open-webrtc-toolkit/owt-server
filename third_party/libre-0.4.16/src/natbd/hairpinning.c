/**
 * @file hairpinning.c  NAT Hairpinning Behaviour discovery
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_mem.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_tcp.h>
#include <re_list.h>
#include <re_stun.h>
#include <re_natbd.h>


#define DEBUG_MODULE "natbd_hairpinning"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/*
   Diagnosing NAT Hairpinning

   STUN Binding Requests allow a a client to determine whether it is
   behind a NAT that support hairpinning of datagrams.  To perform this
   test, the client first sends a Binding Request to its STUN server to
   determine its mapped address.  The client then sends a STUN Binding
   Request to this mapped address from a different port.  If the client
   receives its own request, the NAT hairpins datagrams.  This test
   applies to UDP, TCP, or TCP/TLS connections.

 */


/** Defines NAT Hairpinning Behaviour Discovery */
struct nat_hairpinning {
	struct stun *stun;       /**< STUN Client             */
	int proto;               /**< IP Protocol             */
	struct sa srv;           /**< Server address and port */
	struct udp_sock *us;     /**< UDP socket              */
	struct tcp_conn *tc;     /**< Client TCP Connection   */
	struct tcp_sock *ts;     /**< Server TCP Socket       */
	struct tcp_conn *tc2;    /**< Server TCP Connection   */
	nat_hairpinning_h *hph;  /**< Result handler          */
	void *arg;               /**< Handler argument        */
};


static void hairpinning_destructor(void *data)
{
	struct nat_hairpinning *nh = data;

	mem_deref(nh->us);
	mem_deref(nh->tc);
	mem_deref(nh->ts);
	mem_deref(nh->tc2);
	mem_deref(nh->stun);
}


static void msg_recv(struct nat_hairpinning *nh, int proto, void *sock,
		     const struct sa *src, struct mbuf *mb)
{
	struct stun_unknown_attr ua;
	struct stun_msg *msg;

	if (0 != stun_msg_decode(&msg, mb, &ua))
		return;

	switch (stun_msg_class(msg)) {

	case STUN_CLASS_REQUEST:
		(void)stun_reply(proto, sock, src, 0, msg, NULL, 0, false, 3,
				 STUN_ATTR_XOR_MAPPED_ADDR, src,
				 STUN_ATTR_MAPPED_ADDR, src,
				 STUN_ATTR_SOFTWARE, stun_software);
		break;

	case STUN_CLASS_ERROR_RESP:
	case STUN_CLASS_SUCCESS_RESP:
		(void)stun_ctrans_recv(nh->stun, msg, &ua);
		break;

	default:
		DEBUG_WARNING("unknown class 0x%04x\n", stun_msg_class(msg));
		break;
	}

	mem_deref(msg);
}


static void udp_recv_handler(const struct sa *src, struct mbuf *mb,
			     void *arg)
{
	struct nat_hairpinning *nh = arg;

	msg_recv(nh, IPPROTO_UDP, nh->us, src, mb);
}


static void stun_response_handler2(int err, uint16_t scode, const char *reason,
				   const struct stun_msg *msg, void *arg)
{
	struct nat_hairpinning *nh = arg;
	(void)reason;
	(void)msg;

	if (err || scode) {
		nh->hph(0, false, nh->arg);
		return;
	}

	/* Hairpinning supported */
	nh->hph(0, true, nh->arg);
}


static int hairpin_send(struct nat_hairpinning *nh, const struct sa *srv)
{
	return stun_request(NULL, nh->stun, nh->proto, NULL,
			    srv, 0, STUN_METHOD_BINDING, NULL, 0, false,
			    stun_response_handler2, nh, 1,
			    STUN_ATTR_SOFTWARE, stun_software);
}


/*
 * TCP Connections: STUN Client2 to Embedded STUN Server
 */


static void tcp_recv_handler2(struct mbuf *mb, void *arg)
{
	struct nat_hairpinning *nh = arg;

	msg_recv(nh, IPPROTO_TCP, nh->tc2, NULL, mb);
}


static void tcp_close_handler2(int err, void *arg)
{
	struct nat_hairpinning *nh = arg;

	if (err)
		nh->hph(err, false, nh->arg);
}


static void stun_response_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct nat_hairpinning *nh = arg;
	const struct stun_attr *attr;
	(void)reason;

	if (err) {
		nh->hph(err, false, nh->arg);
		return;
	}

	attr = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
	if (!attr)
		attr = stun_msg_attr(msg, STUN_ATTR_MAPPED_ADDR);

	if (scode || !attr) {
		nh->hph(EBADMSG, false, nh->arg);
		return;
	}

	/* Send hairpinning test message */
	err = hairpin_send(nh, &attr->v.sa);
	if (err) {
		DEBUG_WARNING("hairpin_send: (%m)\n", err);
	}

	if (err)
		nh->hph(err, false, nh->arg);
}


static int mapped_send(struct nat_hairpinning *nh)
{
	return stun_request(NULL, nh->stun, nh->proto, nh->us ?
			    (void *)nh->us : (void *)nh->tc,
			    &nh->srv, 0, STUN_METHOD_BINDING, NULL, 0, false,
			    stun_response_handler, nh, 1,
			    STUN_ATTR_SOFTWARE, stun_software);
}


static void tcp_conn_handler(const struct sa *peer, void *arg)
{
	struct nat_hairpinning *nh = arg;
	int err;

	(void)peer;

	err = tcp_accept(&nh->tc2, nh->ts, NULL, tcp_recv_handler2,
			 tcp_close_handler2, nh);
	if (err) {
		DEBUG_WARNING("TCP conn: tcp_accept: %m\n", err);
	}
}


/*
 * TCP Connection: STUN Client to STUN Server
 */

static void tcp_estab_handler(void *arg)
{
	struct nat_hairpinning *nh = arg;
	int err;

	err = mapped_send(nh);
	if (err) {
		DEBUG_WARNING("TCP established: mapped_send (%m)\n", err);
		nh->hph(err, false, nh->arg);
	}
}


static void tcp_recv_handler(struct mbuf *mb, void *arg)
{
	struct nat_hairpinning *nh = arg;
	int err;

	err = stun_recv(nh->stun, mb);
	if (err && ENOENT != err) {
		DEBUG_WARNING("stun recv: %m\n", err);
	}
}


static void tcp_close_handler(int err, void *arg)
{
	struct nat_hairpinning *nh = arg;

	if (err)
		nh->hph(err, false, nh->arg);
}


/**
 * Allocate a new NAT Hairpinning discovery session
 *
 * @param nhp      Pointer to allocated NAT Hairpinning object
 * @param proto    Transport protocol
 * @param srv      STUN Server IP address and port number
 * @param proto    Transport protocol
 * @param conf     STUN configuration (Optional)
 * @param hph      Hairpinning result handler
 * @param arg      Handler argument
 *
 * @return 0 if success, errorcode if failure
 */
int nat_hairpinning_alloc(struct nat_hairpinning **nhp,
			  const struct sa *srv, int proto,
			  const struct stun_conf *conf,
			  nat_hairpinning_h *hph, void *arg)
{
	struct nat_hairpinning *nh;
	struct sa local;
	int err;

	if (!srv || !hph)
		return EINVAL;

	nh = mem_zalloc(sizeof(*nh), hairpinning_destructor);
	if (!nh)
		return ENOMEM;

	err = stun_alloc(&nh->stun, conf, NULL, NULL);
	if (err)
		goto out;

	sa_cpy(&nh->srv, srv);
	nh->proto = proto;
	nh->hph   = hph;
	nh->arg   = arg;

	switch (proto) {

	case IPPROTO_UDP:
		err = udp_listen(&nh->us, NULL, udp_recv_handler, nh);
		break;

	case IPPROTO_TCP:
		sa_set_in(&local, 0, 0);

		/*
		 * Part I - Allocate and bind all sockets
		 */
		err = tcp_sock_alloc(&nh->ts, &local, tcp_conn_handler, nh);
		if (err)
			break;

		err = tcp_conn_alloc(&nh->tc, &nh->srv,
				     tcp_estab_handler, tcp_recv_handler,
				     tcp_close_handler, nh);
		if (err)
			break;

		err = tcp_sock_bind(nh->ts, &local);
		if (err)
			break;

		err = tcp_sock_local_get(nh->ts, &local);
		if (err)
			break;

		err = tcp_conn_bind(nh->tc, &local);
		if (err)
			break;

		/*
		 * Part II - Listen and connect all sockets
		 */
		err = tcp_sock_listen(nh->ts, 5);
		break;

	default:
		err = EPROTONOSUPPORT;
		break;
	}

 out:
	if (err)
		mem_deref(nh);
	else
		*nhp = nh;

	return err;
}


/**
 * Start a new NAT Hairpinning discovery session
 *
 * @param nh NAT Hairpinning object
 *
 * @return 0 if success, errorcode if failure
 */
int nat_hairpinning_start(struct nat_hairpinning *nh)
{
	if (!nh)
		return EINVAL;

	switch (nh->proto) {

	case IPPROTO_UDP:
		return mapped_send(nh);

	case IPPROTO_TCP:
		return tcp_conn_connect(nh->tc, &nh->srv);

	default:
		return EPROTONOSUPPORT;
	}
}
