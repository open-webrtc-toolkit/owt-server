/**
 * @file re_tcp.h  Interface to Transport Control Protocol
 *
 * Copyright (C) 2010 Creytiv.com
 */
struct sa;
struct tcp_sock;
struct tcp_conn;


/**
 * Defines the incoming TCP connection handler
 *
 * @param peer Network address of peer
 * @param arg  Handler argument
 */
typedef void (tcp_conn_h)(const struct sa *peer, void *arg);

/**
 * Defines the TCP connection established handler
 *
 * @param arg Handler argument
 */
typedef void (tcp_estab_h)(void *arg);

/**
 * Defines the TCP connection data send handler
 *
 * @param arg Handler argument
 */
typedef void (tcp_send_h)(void *arg);

/**
 * Defines the TCP connection data receive handler
 *
 * @param mb  Buffer with data
 * @param arg Handler argument
 */
typedef void (tcp_recv_h)(struct mbuf *mb, void *arg);

/**
 * Defines the TCP connection close handler
 *
 * @param err Error code
 * @param arg Handler argument
 */
typedef void (tcp_close_h)(int err, void *arg);


/* TCP Socket */
int  tcp_sock_alloc(struct tcp_sock **tsp, const struct sa *local,
		    tcp_conn_h *ch, void *arg);
int  tcp_sock_bind(struct tcp_sock *ts, const struct sa *local);
int  tcp_sock_listen(struct tcp_sock *ts, int backlog);
int  tcp_accept(struct tcp_conn **tcp, struct tcp_sock *ts, tcp_estab_h *eh,
		tcp_recv_h *rh, tcp_close_h *ch, void *arg);
void tcp_reject(struct tcp_sock *ts);
int  tcp_sock_local_get(const struct tcp_sock *ts, struct sa *local);


/* TCP Connection */
int  tcp_conn_alloc(struct tcp_conn **tcp, const struct sa *peer,
		    tcp_estab_h *eh, tcp_recv_h *rh, tcp_close_h *ch,
		    void *arg);
int  tcp_conn_bind(struct tcp_conn *tc, const struct sa *local);
int  tcp_conn_connect(struct tcp_conn *tc, const struct sa *peer);
int  tcp_send(struct tcp_conn *tc, struct mbuf *mb);
int  tcp_set_send(struct tcp_conn *tc, tcp_send_h *sendh);
void tcp_set_handlers(struct tcp_conn *tc, tcp_estab_h *eh, tcp_recv_h *rh,
		      tcp_close_h *ch, void *arg);
void tcp_conn_rxsz_set(struct tcp_conn *tc, size_t rxsz);
void tcp_conn_txqsz_set(struct tcp_conn *tc, size_t txqsz);
int  tcp_conn_local_get(const struct tcp_conn *tc, struct sa *local);
int  tcp_conn_peer_get(const struct tcp_conn *tc, struct sa *peer);
int  tcp_conn_fd(const struct tcp_conn *tc);
size_t tcp_conn_txqsz(const struct tcp_conn *tc);


/* High-level API */
int  tcp_listen(struct tcp_sock **tsp, const struct sa *local,
		tcp_conn_h *ch, void *arg);
int  tcp_connect(struct tcp_conn **tcp, const struct sa *peer,
		 tcp_estab_h *eh, tcp_recv_h *rh, tcp_close_h *ch, void *arg);
int  tcp_local_get(const struct tcp_sock *ts, struct sa *local);


#ifdef __SYMBIAN32__
struct RSocketServ;
struct RConnection;
void tcp_rconn_set(struct RSocketServ *sockSrv, struct RConnection *rconn);
#endif


/* Helper API */
typedef bool (tcp_helper_estab_h)(int *err, bool active, void *arg);
typedef bool (tcp_helper_send_h)(int *err, struct mbuf *mb, void *arg);
typedef bool (tcp_helper_recv_h)(int *err, struct mbuf *mb, bool *estab,
				 void *arg);

struct tcp_helper;


int tcp_register_helper(struct tcp_helper **thp, struct tcp_conn *tc,
			int layer,
			tcp_helper_estab_h *eh, tcp_helper_send_h *sh,
			tcp_helper_recv_h *rh, void *arg);
int tcp_send_helper(struct tcp_conn *tc, struct mbuf *mb,
		    struct tcp_helper *th);
