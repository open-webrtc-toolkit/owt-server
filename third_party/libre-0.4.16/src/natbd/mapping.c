/**
 * @file mapping.c  NAT Mapping Behaviour discovery
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_sa.h>
#include <re_udp.h>
#include <re_tcp.h>
#include <re_list.h>
#include <re_stun.h>
#include <re_natbd.h>


#define DEBUG_MODULE "natbd_mapping"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/* Determining NAT Mapping Behavior

   This will require at most three tests.  In test I, the client
   performs the UDP connectivity test.  The server will return its
   alternate address and port in OTHER-ADDRESS in the binding response.
   If OTHER-ADDRESS is not returned, the server does not support this
   usage and this test cannot be run.  The client examines the XOR-
   MAPPED-ADDRESS attribute.  If this address and port are the same as
   the local IP address and port of the socket used to send the request,
   the client knows that it is not NATed and the effective mapping will
   be Endpoint-Independent.

   In test II, the client sends a Binding Request to the alternate
   address, but primary port.  If the XOR-MAPPED-ADDRESS in the Binding
   Response is the same as test I the NAT currently has Endpoint-
   Independent Mapping.  If not, test III is performed: the client sends
   a Binding Request to the alternate address and port.  If the XOR-
   MAPPED-ADDRESS matches test II, the NAT currently has Address-
   Dependent Mapping; if it doesn't match it currently has Address and
   Port-Dependent Mapping.
 */


/** Defines a NAT Mapping Behaviour Discovery session */
struct nat_mapping {
	struct stun *stun;        /**< STUN Instance             */
	struct udp_sock *us;      /**< UDP socket                */
	struct tcp_conn *tc;      /**< TCP connection            */
	struct sa laddr;          /**< Local IP address and port */
	struct sa map[3];         /**< XOR Mapped address/ports  */
	struct sa srv;            /**< STUN server address/port  */
	nat_mapping_h *mh;        /**< Result handler            */
	void *arg;                /**< Handler argument          */
	int proto;                /**< IP Protocol               */
	uint32_t test_phase;      /**< State machine             */
	struct tcp_conn *tcv[3];  /**< TCP Connections           */
};


static int mapping_send(struct nat_mapping *nm);


static void stun_response_handler(int err, uint16_t scode, const char *reason,
				  const struct stun_msg *msg, void *arg)
{
	struct nat_mapping *nm = arg;
	struct stun_attr *map, *other;

	if (err) {
		DEBUG_WARNING("stun_response_handler: (%m)\n", err);
		nm->mh(err, NAT_TYPE_UNKNOWN, nm->arg);
		return;
	}

	switch (scode) {

	case 0:
		other = stun_msg_attr(msg, STUN_ATTR_OTHER_ADDR);
		map = stun_msg_attr(msg, STUN_ATTR_XOR_MAPPED_ADDR);
		if (!map)
			map = stun_msg_attr(msg, STUN_ATTR_MAPPED_ADDR);

		if (!map || !other) {
			DEBUG_WARNING("missing attributes: %s %s\n",
				      map   ? "" : "MAPPED-ADDR",
				      other ? "" : "OTHER-ADDR");
			nm->mh(EPROTO, NAT_TYPE_UNKNOWN, nm->arg);
			return;
		}

		nm->map[nm->test_phase-1] = map->v.sa;
		break;

	default:
		DEBUG_WARNING("Binding Error Resp: %u %s\n", scode, reason);
		nm->mh(EPROTO, NAT_TYPE_UNKNOWN, nm->arg);
		return;
	}

	switch (nm->test_phase) {

	case 1:
		/* Test I completed */
		if (sa_cmp(&nm->laddr, &nm->map[0], SA_ALL)) {
			nm->mh(0, NAT_TYPE_ENDP_INDEP, nm->arg);
			return;
		}

		/* Start Test II - the client sends a Binding Request to the
		   alternate *address* */
		++nm->test_phase;

		sa_set_port(&other->v.other_addr, sa_port(&nm->srv));
		sa_cpy(&nm->srv, &other->v.other_addr);

		err = mapping_send(nm);
		if (err) {
			DEBUG_WARNING("stunc_request_send: (%m)\n", err);
			nm->mh(err, NAT_TYPE_UNKNOWN, nm->arg);
		}
		break;

	case 2:
		/* Test II completed */
		if (sa_cmp(&nm->map[0], &nm->map[1], SA_ALL)) {
			nm->mh(0, NAT_TYPE_ENDP_INDEP, nm->arg);
			return;
		}

		/* Start Test III - the client sends a Binding Request
		   to the alternate address and port */
		++nm->test_phase;

		sa_set_port(&nm->srv, sa_port(&other->v.other_addr));
		err = mapping_send(nm);
		if (err) {
			DEBUG_WARNING("stunc_request_send: (%m)\n", err);
			nm->mh(err, NAT_TYPE_UNKNOWN, nm->arg);
		}
		break;

	case 3:
		/* Test III completed */
		if (sa_cmp(&nm->map[1], &nm->map[2], SA_ALL)) {
			nm->mh(0, NAT_TYPE_ADDR_DEP, nm->arg);
		}
		else {
			nm->mh(0, NAT_TYPE_ADDR_PORT_DEP, nm->arg);
		}
		++nm->test_phase;
		break;

	default:
		DEBUG_WARNING("invalid test phase %d\n", nm->test_phase);
		nm->mh(EINVAL, NAT_TYPE_UNKNOWN, nm->arg);
		break;
	}
}


static int mapping_send(struct nat_mapping *nm)
{
	switch (nm->proto) {

	case IPPROTO_UDP:
		return stun_request(NULL, nm->stun, IPPROTO_UDP, nm->us,
				    &nm->srv, 0, STUN_METHOD_BINDING, NULL, 0,
				    false, stun_response_handler, nm, 1,
				    STUN_ATTR_SOFTWARE, stun_software);

	case IPPROTO_TCP:
		nm->tc = mem_deref(nm->tc);
		nm->tc = mem_ref(nm->tcv[nm->test_phase-1]);
		return tcp_conn_connect(nm->tc, &nm->srv);

	default:
		return EPROTONOSUPPORT;
	}
}


static void udp_recv_handler(const struct sa *src, struct mbuf *mb,
			     void *arg)
{
	struct nat_mapping *nm = arg;
	int err;
	(void)src;

	err = stun_recv(nm->stun, mb);
	if (err && ENOENT != err) {
		DEBUG_WARNING("udp_recv_handler: stunc_recv(): (%m)\n", err);
	}
}


static void mapping_destructor(void *data)
{
	struct nat_mapping *nm = data;
	int i;

	mem_deref(nm->us);
	mem_deref(nm->tc);

	for (i=0; i<3; i++)
		mem_deref(nm->tcv[i]);

	mem_deref(nm->stun);
}


static void tcp_estab_handler(void *arg)
{
	struct nat_mapping *nm = arg;
	int err;

	err = stun_request(NULL, nm->stun, IPPROTO_TCP, nm->tc, NULL, 0,
			   STUN_METHOD_BINDING, NULL, 0, false,
			   stun_response_handler, nm, 1,
			   STUN_ATTR_SOFTWARE, stun_software);

	if (err) {
		DEBUG_WARNING("TCP established: mapping_send (%m)\n", err);
		nm->mh(err, NAT_TYPE_UNKNOWN, nm->arg);
	}
}


static void tcp_recv_handler(struct mbuf *mb, void *arg)
{
	struct nat_mapping *nm = arg;
	int err;

	err = stun_recv(nm->stun, mb);
	if (err && ENOENT != err) {
		DEBUG_WARNING("stunc recv: %m\n", err);
	}
}


static void tcp_close_handler(int err, void *arg)
{
	struct nat_mapping *nm = arg;

	DEBUG_NOTICE("TCP Connection Closed (%m)\n", err);

	if (err) {
		nm->mh(err, NAT_TYPE_UNKNOWN, nm->arg);
	}
}


/**
 * Allocate a new NAT Mapping Behaviour Discovery session
 *
 * @param nmp       Pointer to allocated NAT Mapping object
 * @param laddr     Local IP address
 * @param srv       STUN Server IP address and port
 * @param proto     Transport protocol
 * @param conf      STUN configuration (Optional)
 * @param mh        Mapping handler
 * @param arg       Handler argument
 *
 * @return 0 if success, errorcode if failure
 */
int nat_mapping_alloc(struct nat_mapping **nmp, const struct sa *laddr,
		      const struct sa *srv, int proto,
		      const struct stun_conf *conf,
		      nat_mapping_h *mh, void *arg)
{
	struct nat_mapping *nm;
	int i, err;

	if (!nmp || !laddr || !srv || !mh)
		return EINVAL;

	nm = mem_zalloc(sizeof(*nm), mapping_destructor);
	if (!nm)
		return ENOMEM;

	err = stun_alloc(&nm->stun, conf, NULL, NULL);
	if (err)
		goto out;

	nm->proto = proto;
	sa_cpy(&nm->laddr, laddr);

	switch (proto) {

	case IPPROTO_UDP:
		err = udp_listen(&nm->us, &nm->laddr, udp_recv_handler, nm);
		if (err)
			goto out;
		err = udp_local_get(nm->us, &nm->laddr);
		if (err)
			goto out;
		break;

	case IPPROTO_TCP:

		/* Allocate and bind 3 TCP Sockets */
		for (i=0; i<3; i++) {
			err = tcp_conn_alloc(&nm->tcv[i], srv,
					     tcp_estab_handler,
					     tcp_recv_handler,
					     tcp_close_handler, nm);
			if (err)
				goto out;

			err = tcp_conn_bind(nm->tcv[i], &nm->laddr);
			if (err)
				goto out;

			err = tcp_conn_local_get(nm->tcv[i], &nm->laddr);
			if (err)
				goto out;
		}
		break;

	default:
		err = EPROTONOSUPPORT;
		goto out;
	}

	sa_cpy(&nm->srv, srv);
	nm->mh  = mh;
	nm->arg = arg;

	*nmp = nm;

 out:
	if (err)
		mem_deref(nm);
	return err;
}


/**
 * Start a new NAT Mapping Behaviour Discovery session
 *
 * @param nm NAT Mapping object
 *
 * @return 0 if success, errorcode if failure
 */
int nat_mapping_start(struct nat_mapping *nm)
{
	if (!nm)
		return EINVAL;

	nm->test_phase = 1;

	return mapping_send(nm);
}
