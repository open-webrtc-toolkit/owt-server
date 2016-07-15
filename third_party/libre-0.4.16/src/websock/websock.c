/**
 * @file websock.c  Implementation of The WebSocket Protocol
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_tmr.h>
#include <re_srtp.h>
#include <re_tcp.h>
#include <re_tls.h>
#include <re_msg.h>
#include <re_http.h>
#include <re_base64.h>
#include <re_sha.h>
#include <re_sys.h>
#include <re_websock.h>


enum {
	TIMEOUT_CLOSE = 10000,
	BUFSIZE_MAX   = 131072,
};

enum websock_state {
	ACCEPTING = 0,
	CONNECTING,
	OPEN,
	CLOSING,
	CLOSED,
};

struct websock {
	websock_shutdown_h *shuth;
	void *arg;
	bool shutdown;
};

struct websock_conn {
	struct tmr tmr;
	struct sa peer;
	char nonce[24];
	struct websock *sock;
	struct tcp_conn *tc;
	struct tls_conn *sc;
	struct mbuf *mb;
	struct http_req *req;
	websock_estab_h *estabh;
	websock_recv_h *recvh;
	websock_close_h *closeh;
	void *arg;
	enum websock_state state;
	unsigned kaint;
	bool active;
};


static const char magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


static void timeout_handler(void *arg);


static void dummy_recv_handler(const struct websock_hdr *hdr, struct mbuf *mb,
			       void *arg)
{
	(void)hdr;
	(void)mb;
	(void)arg;
}


static void internal_close_handler(int err, void *arg)
{
	struct websock_conn *conn = arg;
	(void)err;

	mem_deref(conn);
}


static void sock_destructor(void *arg)
{
	struct websock *sock = arg;

	if (sock->shutdown) {
		sock->shutdown = false;
		mem_ref(sock);
		if (sock->shuth)
			sock->shuth(sock->arg);
		return;
	}
}


static void conn_destructor(void *arg)
{
	struct websock_conn *conn = arg;

	if (conn->state == OPEN)
		(void)websock_close(conn, WEBSOCK_GOING_AWAY, "Going Away");

	if (conn->state == CLOSING) {

		conn->recvh  = dummy_recv_handler;
		conn->closeh = internal_close_handler;
		conn->arg    = conn;

		tmr_start(&conn->tmr, TIMEOUT_CLOSE, timeout_handler, conn);

		/* important: the hack below depends on this */
		mem_ref(conn);
		return;
	}

	tmr_cancel(&conn->tmr);
	mem_deref(conn->sc);
	mem_deref(conn->tc);
	mem_deref(conn->mb);
	mem_deref(conn->req);
	mem_deref(conn->sock);
}


static void conn_close(struct websock_conn *conn, int err)
{
	tmr_cancel(&conn->tmr);
	conn->sc = mem_deref(conn->sc);
	conn->tc = mem_deref(conn->tc);
	conn->state = CLOSED;

	conn->closeh(err, conn->arg);
}


static void timeout_handler(void *arg)
{
	struct websock_conn *conn = arg;

	conn_close(conn, ETIMEDOUT);
}


static void keepalive_handler(void *arg)
{
	struct websock_conn *conn = arg;

	tmr_start(&conn->tmr, conn->kaint, keepalive_handler, conn);

	(void)websock_send(conn, WEBSOCK_PING, NULL);
}


static enum websock_scode websock_err2scode(int err)
{
	switch (err) {

	case EOVERFLOW: return WEBSOCK_MESSAGE_TOO_BIG;
	case EPROTO:    return WEBSOCK_PROTOCOL_ERROR;
	case EBADMSG:   return WEBSOCK_PROTOCOL_ERROR;
	default:        return WEBSOCK_INTERNAL_ERROR;
	}
}


static int websock_decode(struct websock_hdr *hdr, struct mbuf *mb)
{
	uint8_t v, *p;
	size_t i;

	if (mbuf_get_left(mb) < 2)
		return ENODATA;

	v = mbuf_read_u8(mb);
	hdr->fin    = v>>7 & 0x1;
	hdr->rsv1   = v>>6 & 0x1;
	hdr->rsv2   = v>>5 & 0x1;
	hdr->rsv3   = v>>4 & 0x1;
	hdr->opcode = v    & 0x0f;

	v = mbuf_read_u8(mb);
	hdr->mask = v>>7 & 0x1;
	hdr->len  = v    & 0x7f;

	if (hdr->len == 126) {

		if (mbuf_get_left(mb) < 2)
			return ENODATA;

		hdr->len = ntohs(mbuf_read_u16(mb));
	}
	else if (hdr->len == 127) {

		if (mbuf_get_left(mb) < 8)
			return ENODATA;

		hdr->len = sys_ntohll(mbuf_read_u64(mb));
	}

	if (hdr->mask) {

		if (mbuf_get_left(mb) < (4 + hdr->len))
			return ENODATA;

		hdr->mkey[0] = mbuf_read_u8(mb);
		hdr->mkey[1] = mbuf_read_u8(mb);
		hdr->mkey[2] = mbuf_read_u8(mb);
		hdr->mkey[3] = mbuf_read_u8(mb);

		for (i=0, p=mbuf_buf(mb); i<hdr->len; i++)
			p[i] = p[i] ^ hdr->mkey[i%4];
	}
	else {
		if (mbuf_get_left(mb) < hdr->len)
			return ENODATA;
	}

	return 0;
}


static void recv_handler(struct mbuf *mb, void *arg)
{
	struct websock_conn *conn = arg;
	int err = 0;

	if (conn->mb) {

		const size_t len = mbuf_get_left(mb), pos = conn->mb->pos;

		if ((mbuf_get_left(conn->mb) + len) > BUFSIZE_MAX) {
			err = EOVERFLOW;
			goto out;
		}

		conn->mb->pos = conn->mb->end;

		err = mbuf_write_mem(conn->mb, mbuf_buf(mb), len);
		if (err)
			goto out;

		conn->mb->pos = pos;
	}
	else {
		conn->mb = mem_ref(mb);
	}

	while (conn->mb) {

		struct websock_hdr hdr;
		size_t pos, end;

		pos = conn->mb->pos;

		err = websock_decode(&hdr, conn->mb);
		if (err) {
			if (err == ENODATA) {
				conn->mb->pos = pos;
				err = 0;
				break;
			}

			goto out;
		}

		if (conn->active == hdr.mask) {
			err = EPROTO;
			goto out;
		}

		if (hdr.rsv1 || hdr.rsv2 || hdr.rsv3) {
			err = EPROTO;
			goto out;
		}

		mb = conn->mb;

		end     = mb->end;
		mb->end = mb->pos + (size_t)hdr.len;

		if (end > mb->end) {
			struct mbuf *mbn = mbuf_alloc(end - mb->end);
			if (!mbn) {
				err = ENOMEM;
				goto out;
			}

			(void)mbuf_write_mem(mbn, mb->buf + mb->end,
					     end - mb->end);
			mbn->pos = 0;

			conn->mb = mbn;
		}
		else {
			conn->mb = NULL;
		}

		switch (hdr.opcode) {

		case WEBSOCK_CONT:
		case WEBSOCK_TEXT:
		case WEBSOCK_BIN:
			mem_ref(conn);
			conn->recvh(&hdr, mb, conn->arg);

			if (mem_nrefs(conn) == 1) {

				if (conn->state == OPEN)
					(void)websock_close(conn,
						            WEBSOCK_GOING_AWAY,
							    "Going Away");

				/*
				 * This is a hack. We enforce CLOSING
				 * state so we know the connection will
				 * continue to live.
				 */
				conn->state = CLOSING;
			}
			mem_deref(conn);
			break;

		case WEBSOCK_CLOSE:
			if (conn->state == OPEN)
				(void)websock_send(conn, WEBSOCK_CLOSE, "%b",
					     mbuf_buf(mb), mbuf_get_left(mb));
			conn_close(conn, 0);
			mem_deref(mb);
			return;

		case WEBSOCK_PING:
			(void)websock_send(conn, WEBSOCK_PONG, "%b",
					   mbuf_buf(mb), mbuf_get_left(mb));
			break;

		case WEBSOCK_PONG:
			break;

		default:
			mem_deref(mb);
			err = EPROTO;
			goto out;
		}

		mem_deref(mb);
	}

 out:
	if (err) {
		(void)websock_close(conn, websock_err2scode(err), NULL);
		conn_close(conn, err);
	}
}


static void close_handler(int err, void *arg)
{
	struct websock_conn *conn = arg;

	conn_close(conn, err);
}


static int accept_print(struct re_printf *pf, const struct pl *key)
{
	uint8_t digest[SHA_DIGEST_LENGTH];
	SHA_CTX ctx;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, key->p, key->l);
	SHA1_Update(&ctx, magic, sizeof(magic)-1);
	SHA1_Final(digest, &ctx);

	return base64_print(pf, digest, sizeof(digest));
}


static void http_resp_handler(int err, const struct http_msg *msg, void *arg)
{
	struct websock_conn *conn = arg;
	const struct http_hdr *hdr;
	struct pl key;
	char buf[32];

	if (err || msg->scode != 101)
		goto fail;

	if (!http_msg_hdr_has_value(msg, HTTP_HDR_UPGRADE, "websocket"))
		goto fail;

	if (!http_msg_hdr_has_value(msg, HTTP_HDR_CONNECTION, "Upgrade"))
		goto fail;

	hdr = http_msg_hdr(msg, HTTP_HDR_SEC_WEBSOCKET_ACCEPT);
	if (!hdr)
		goto fail;

	key.p = conn->nonce;
	key.l = sizeof(conn->nonce);

	if (re_snprintf(buf, sizeof(buf), "%H", accept_print, &key) < 0)
		goto fail;

	if (pl_strcmp(&hdr->val, buf))
		goto fail;

	/* here we are ok */

	conn->state = OPEN;
	conn->tc = mem_ref(http_req_tcp(conn->req));
	conn->sc = mem_ref(http_req_tls(conn->req));
	(void)tcp_conn_peer_get(conn->tc, &conn->peer);

	tcp_set_handlers(conn->tc, NULL, recv_handler, close_handler, conn);
	conn->req = mem_deref(conn->req);

	if (conn->kaint)
		tmr_start(&conn->tmr, conn->kaint, keepalive_handler, conn);

	conn->estabh(conn->arg);
	return;

 fail:
	conn_close(conn, err ? err : EPROTO);
}


/* dummy HTTP data handler, this must be here so that HTTP client
 * is not closing the underlying TCP-connection (which we need ..)
 */
static void http_data_handler(struct mbuf *mb, void *arg)
{
	(void)mb;
	(void)arg;
}


int websock_connect(struct websock_conn **connp, struct websock *sock,
		    struct http_cli *cli, const char *uri, unsigned kaint,
		    websock_estab_h *estabh, websock_recv_h *recvh,
		    websock_close_h *closeh, void *arg,
		    const char *fmt, ...)
{
	struct websock_conn *conn;
	uint8_t nonce[16];
	va_list ap;
	size_t len;
	int err;

	if (!connp || !sock || !cli || !uri || !estabh || !recvh || !closeh)
		return EINVAL;

	conn = mem_zalloc(sizeof(*conn), conn_destructor);
	if (!conn)
		return ENOMEM;

	/* The nonce MUST be selected randomly for each connection */
	rand_bytes(nonce, sizeof(nonce));

	len = sizeof(conn->nonce);

	err = base64_encode(nonce, sizeof(nonce), conn->nonce, &len);
	if (err)
		goto out;

	conn->sock   = mem_ref(sock);
	conn->kaint  = kaint;
	conn->estabh = estabh;
	conn->recvh  = recvh;
	conn->closeh = closeh;
	conn->arg    = arg;
	conn->state  = CONNECTING;
	conn->active = true;

	/* Protocol Handshake */
	va_start(ap, fmt);
	err = http_request(&conn->req, cli, "GET", uri,
			   http_resp_handler, http_data_handler, conn,
			   "Upgrade: websocket\r\n"
			   "Connection: upgrade\r\n"
			   "Sec-WebSocket-Key: %b\r\n"
			   "Sec-WebSocket-Version: 13\r\n"
			   "%v"
			   "\r\n",
			   conn->nonce, sizeof(conn->nonce),
			   fmt, &ap);
	va_end(ap);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(conn);
	else
		*connp = conn;

	return err;
}


int websock_accept(struct websock_conn **connp, struct websock *sock,
		   struct http_conn *htconn, const struct http_msg *msg,
		   unsigned kaint, websock_recv_h *recvh,
		   websock_close_h *closeh, void *arg)
{
	const struct http_hdr *key;
	struct websock_conn *conn;
	int err;

	if (!connp || !sock || !htconn || !msg || !recvh || !closeh)
		return EINVAL;

	if (!http_msg_hdr_has_value(msg, HTTP_HDR_UPGRADE, "websocket"))
		return EBADMSG;

	if (!http_msg_hdr_has_value(msg, HTTP_HDR_CONNECTION, "Upgrade"))
		return EBADMSG;

	if (!http_msg_hdr_has_value(msg, HTTP_HDR_SEC_WEBSOCKET_VERSION, "13"))
		return EBADMSG;

	key = http_msg_hdr(msg, HTTP_HDR_SEC_WEBSOCKET_KEY);
	if (!key)
		return EBADMSG;

	conn = mem_zalloc(sizeof(*conn), conn_destructor);
	if (!conn)
		return ENOMEM;

	err = http_reply(htconn, 101, "Switching Protocols",
			 "Upgrade: websocket\r\n"
			 "Connection: Upgrade\r\n"
			 "Sec-WebSocket-Accept: %H\r\n"
			 "\r\n",
			 accept_print, &key->val);
	if (err)
		goto out;

	sa_cpy(&conn->peer, http_conn_peer(htconn));
	conn->sock   = mem_ref(sock);
	conn->tc     = mem_ref(http_conn_tcp(htconn));
	conn->sc     = mem_ref(http_conn_tls(htconn));
	conn->kaint  = kaint;
	conn->recvh  = recvh;
	conn->closeh = closeh;
	conn->arg    = arg;
	conn->state  = OPEN;
	conn->active = false;

	tcp_set_handlers(conn->tc, NULL, recv_handler, close_handler, conn);
	http_conn_close(htconn);

	if (conn->kaint)
		tmr_start(&conn->tmr, conn->kaint, keepalive_handler, conn);

 out:
	if (err)
		mem_deref(conn);
	else
		*connp = conn;

	return err;
}


static int websock_encode(struct mbuf *mb, bool fin,
			  enum websock_opcode opcode, bool mask, size_t len)
{
	int err;

	err = mbuf_write_u8(mb, (fin<<7) | (opcode & 0x0f));

	if (len > 0xffff) {
		err |= mbuf_write_u8(mb, (mask<<7) | 127);
		err |= mbuf_write_u64(mb, sys_htonll(len));
	}
	else if (len > 125) {
		err |= mbuf_write_u8(mb, (mask<<7) | 126);
		err |= mbuf_write_u16(mb, htons(len));
	}
	else {
		err |= mbuf_write_u8(mb, (mask<<7) | len);
	}

	if (mask) {
		uint8_t mkey[4];
		uint8_t *p;
		size_t i;

		rand_bytes(mkey, sizeof(mkey));

		err |= mbuf_write_mem(mb, mkey, sizeof(mkey));

		for (i=0, p=mbuf_buf(mb); i<len; i++)
			p[i] = p[i] ^ mkey[i%4];
	}

	return err;
}


static int websock_vsend(struct websock_conn *conn, enum websock_opcode opcode,
			 enum websock_scode scode, const char *fmt, va_list ap)
{
	const size_t hsz = conn->active ? 14 : 10;
	size_t len, start;
	struct mbuf *mb;
	int err = 0;

	if (conn->state != OPEN)
		return ENOTCONN;

	mb = mbuf_alloc(2048);
	if (!mb)
		return ENOMEM;

	mb->pos = hsz;

	if (scode)
		err |= mbuf_write_u16(mb, htons(scode));
	if (fmt)
		err |= mbuf_vprintf(mb, fmt, ap);
	if (err)
		goto out;

	len = mb->pos - hsz;

	if (len > 0xffff)
		start = mb->pos = 0;
	else if (len > 125)
		start = mb->pos = 6;
	else
		start = mb->pos = 8;

	err = websock_encode(mb, true, opcode, conn->active, len);
	if (err)
		goto out;

	mb->pos = start;

	err = tcp_send(conn->tc, mb);
	if (err)
		goto out;

 out:
	mem_deref(mb);

	return err;
}


int websock_send(struct websock_conn *conn, enum websock_opcode opcode,
		 const char *fmt, ...)
{
	va_list ap;
	int err;

	if (!conn)
		return EINVAL;

	va_start(ap, fmt);
	err = websock_vsend(conn, opcode, 0, fmt, ap);
	va_end(ap);

	return err;
}


int websock_close(struct websock_conn *conn, enum websock_scode scode,
		  const char *fmt, ...)
{
	va_list ap;
	int err;

	if (!conn)
		return EINVAL;

	if (!scode)
		fmt = NULL;

	va_start(ap, fmt);
	err = websock_vsend(conn, WEBSOCK_CLOSE, scode, fmt, ap);
	va_end(ap);

	if (!err)
		conn->state = CLOSING;

	return err;
}


const struct sa *websock_peer(const struct websock_conn *conn)
{
	return conn ? &conn->peer : NULL;
}


int websock_alloc(struct websock **sockp, websock_shutdown_h *shuth, void *arg)
{
	struct websock *sock;

	if (!sockp)
		return EINVAL;

	sock = mem_zalloc(sizeof(*sock), sock_destructor);
	if (!sock)
		return ENOMEM;

	sock->shuth = shuth;
	sock->arg   = arg;

	*sockp = sock;

	return 0;
}


void websock_shutdown(struct websock *sock)
{
	if (!sock || sock->shutdown)
		return;

	sock->shutdown = true;
	mem_deref(sock);
}
