/**
 * @file re_websock.h  The WebSocket Protocol
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	WEBSOCK_VERSION = 13,
};

enum websock_opcode {
	/* Data frames */
	WEBSOCK_CONT  = 0x0,
	WEBSOCK_TEXT  = 0x1,
	WEBSOCK_BIN   = 0x2,
	/* Control frames */
	WEBSOCK_CLOSE = 0x8,
	WEBSOCK_PING  = 0x9,
	WEBSOCK_PONG  = 0xa,
};

enum websock_scode {
	WEBSOCK_NORMAL_CLOSURE   = 1000,
	WEBSOCK_GOING_AWAY       = 1001,
	WEBSOCK_PROTOCOL_ERROR   = 1002,
	WEBSOCK_UNSUPPORTED_DATA = 1003,
	WEBSOCK_INVALID_PAYLOAD  = 1007,
	WEBSOCK_POLICY_VIOLATION = 1008,
	WEBSOCK_MESSAGE_TOO_BIG  = 1009,
	WEBSOCK_EXTENSION_ERROR  = 1010,
	WEBSOCK_INTERNAL_ERROR   = 1011,
};

struct websock_hdr {
	unsigned fin:1;
	unsigned rsv1:1;
	unsigned rsv2:1;
	unsigned rsv3:1;
	unsigned opcode:4;
	unsigned mask:1;
	uint64_t len;
	uint8_t mkey[4];
};

struct websock;
struct websock_conn;

typedef void (websock_estab_h)(void *arg);
typedef void (websock_recv_h)(const struct websock_hdr *hdr, struct mbuf *mb,
			      void *arg);
typedef void (websock_close_h)(int err, void *arg);


int websock_connect(struct websock_conn **connp, struct websock *sock,
		    struct http_cli *cli, const char *uri, unsigned kaint,
		    websock_estab_h *estabh, websock_recv_h *recvh,
		    websock_close_h *closeh, void *arg,
		    const char *fmt, ...);
int websock_accept(struct websock_conn **connp, struct websock *sock,
		   struct http_conn *htconn, const struct http_msg *msg,
		   unsigned kaint, websock_recv_h *recvh,
		   websock_close_h *closeh, void *arg);
int websock_send(struct websock_conn *conn, enum websock_opcode opcode,
		 const char *fmt, ...);
int websock_close(struct websock_conn *conn, enum websock_scode scode,
		  const char *fmt, ...);
const struct sa *websock_peer(const struct websock_conn *conn);

typedef void (websock_shutdown_h)(void *arg);

int  websock_alloc(struct websock **sockp, websock_shutdown_h *shuth,
		   void *arg);
void websock_shutdown(struct websock *sock);
