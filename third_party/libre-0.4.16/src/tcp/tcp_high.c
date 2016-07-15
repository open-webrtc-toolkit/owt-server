/**
 * @file tcp_high.c  High-level TCP functions
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_tcp.h>


/**
 * Create and listen on a TCP Socket
 *
 * @param tsp   Pointer to returned TCP Socket
 * @param local Local listen address (NULL for any)
 * @param ch    Incoming connection handler
 * @param arg   Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int tcp_listen(struct tcp_sock **tsp, const struct sa *local,
	       tcp_conn_h *ch, void *arg)
{
	struct tcp_sock *ts = NULL;
	int err;

	if (!tsp)
		return EINVAL;

	err = tcp_sock_alloc(&ts, local, ch, arg);
	if (err)
		goto out;

	err = tcp_sock_bind(ts, local);
	if (err)
		goto out;

	err = tcp_sock_listen(ts, 5);
	if (err)
		goto out;

 out:
	if (err)
		ts = mem_deref(ts);
	else
		*tsp = ts;

	return err;
}


/**
 * Make a TCP Connection to a remote peer
 *
 * @param tcp  Returned TCP Connection object
 * @param peer Network address of peer
 * @param eh   TCP Connection Established handler
 * @param rh   TCP Connection Receive data handler
 * @param ch   TCP Connection close handler
 * @param arg  Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int tcp_connect(struct tcp_conn **tcp, const struct sa *peer,
		tcp_estab_h *eh, tcp_recv_h *rh, tcp_close_h *ch, void *arg)
{
	struct tcp_conn *tc = NULL;
	int err;

	if (!tcp || !peer)
		return EINVAL;

	err = tcp_conn_alloc(&tc, peer, eh,rh, ch, arg);
	if (err)
		goto out;

	err = tcp_conn_connect(tc, peer);
	if (err)
		goto out;

 out:
	if (err)
		tc = mem_deref(tc);
	else
		*tcp = tc;

	return err;
}


/**
 * Get local network address of TCP Socket
 *
 * @param ts    TCP Socket
 * @param local Returned local network address
 *
 * @return 0 if success, otherwise errorcode
 */
int tcp_local_get(const struct tcp_sock *ts, struct sa *local)
{
	return tcp_sock_local_get(ts, local);
}
