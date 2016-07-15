/**
 * @file re_tls.h  Interface to Transport Layer Security
 *
 * Copyright (C) 2010 Creytiv.com
 */


struct tls;
struct tls_conn;
struct tcp_conn;
struct udp_sock;


/** Defines the TLS method */
enum tls_method {
	TLS_METHOD_SSLV23,
	TLS_METHOD_DTLSV1,
	TLS_METHOD_DTLS,      /* DTLS 1.0 and 1.2 */
	TLS_METHOD_DTLSV1_2,  /* DTLS 1.2 */
};

enum tls_fingerprint {
	TLS_FINGERPRINT_SHA1,
	TLS_FINGERPRINT_SHA256,
};


int tls_alloc(struct tls **tlsp, enum tls_method method, const char *keyfile,
	      const char *pwd);
int tls_add_ca(struct tls *tls, const char *capath);
int tls_set_selfsigned(struct tls *tls, const char *cn);
int tls_set_certificate(struct tls *tls, const char *cert, size_t len);
void tls_set_verify_client(struct tls *tls);
int tls_set_srtp(struct tls *tls, const char *suites);
int tls_fingerprint(const struct tls *tls, enum tls_fingerprint type,
		    uint8_t *md, size_t size);

int tls_peer_fingerprint(const struct tls_conn *tc, enum tls_fingerprint type,
			 uint8_t *md, size_t size);
int tls_peer_common_name(const struct tls_conn *tc, char *cn, size_t size);
int tls_peer_verify(const struct tls_conn *tc);
int tls_srtp_keyinfo(const struct tls_conn *tc, enum srtp_suite *suite,
		     uint8_t *cli_key, size_t cli_key_size,
		     uint8_t *srv_key, size_t srv_key_size);
const char *tls_cipher_name(const struct tls_conn *tc);


/* TCP */

int tls_start_tcp(struct tls_conn **ptc, struct tls *tls,
		  struct tcp_conn *tcp, int layer);


/* UDP (DTLS) */

typedef void (dtls_conn_h)(const struct sa *peer, void *arg);
typedef void (dtls_estab_h)(void *arg);
typedef void (dtls_recv_h)(struct mbuf *mb, void *arg);
typedef void (dtls_close_h)(int err, void *arg);

struct dtls_sock;

int dtls_listen(struct dtls_sock **sockp, const struct sa *laddr,
		struct udp_sock *us, uint32_t htsize, int layer,
		dtls_conn_h *connh, void *arg);
struct udp_sock *dtls_udp_sock(struct dtls_sock *sock);
void dtls_set_mtu(struct dtls_sock *sock, size_t mtu);
int dtls_connect(struct tls_conn **ptc, struct tls *tls,
		 struct dtls_sock *sock, const struct sa *peer,
		 dtls_estab_h *estabh, dtls_recv_h *recvh,
		 dtls_close_h *closeh, void *arg);
int dtls_accept(struct tls_conn **ptc, struct tls *tls,
		struct dtls_sock *sock,
		dtls_estab_h *estabh, dtls_recv_h *recvh,
		dtls_close_h *closeh, void *arg);
int dtls_send(struct tls_conn *tc, struct mbuf *mb);
void dtls_set_handlers(struct tls_conn *tc, dtls_estab_h *estabh,
		       dtls_recv_h *recvh, dtls_close_h *closeh, void *arg);
