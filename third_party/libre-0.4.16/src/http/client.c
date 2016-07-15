/**
 * @file http/client.c HTTP Client
 *
 * Copyright (C) 2011 Creytiv.com
 */

#include <string.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_tmr.h>
#include <re_srtp.h>
#include <re_tcp.h>
#include <re_tls.h>
#include <re_dns.h>
#include <re_msg.h>
#include <re_http.h>


enum {
	CONN_TIMEOUT = 30000,
	RECV_TIMEOUT = 30000,
	BUFSIZE_MAX  = 524288,
};

struct http_cli {
	struct dnsc *dnsc;
	struct tls *tls;
};

struct http_req {
	struct sa srvv[16];
	struct tmr tmr;
	struct http_req **reqp;
	struct dns_query *dq;
	struct tls_conn *sc;
	struct tcp_conn *tc;
	struct mbuf *mbreq;
	struct mbuf *mb;
	struct tls *tls;
	char *host;
	http_resp_h *resph;
	http_data_h *datah;
	void *arg;
	unsigned srvc;
	uint16_t port;
	bool secure;
	bool data;
};


static int req_connect(struct http_req *req);


static void cli_destructor(void *arg)
{
	struct http_cli *cli = arg;

	mem_deref(cli->dnsc);
	mem_deref(cli->tls);
}


static void req_destructor(void *arg)
{
	struct http_req *req = arg;

	tmr_cancel(&req->tmr);
	mem_deref(req->dq);
	mem_deref(req->sc);
	mem_deref(req->tc);
	mem_deref(req->mbreq);
	mem_deref(req->mb);
	mem_deref(req->tls);
	mem_deref(req->host);
}


static void req_close(struct http_req *req, int err,
		      const struct http_msg *msg)
{
	tmr_cancel(&req->tmr);
	req->dq = mem_deref(req->dq);
	req->sc = mem_deref(req->sc);
	req->tc = mem_deref(req->tc);
	req->datah = NULL;

	if (req->reqp) {
		*req->reqp = NULL;
		req->reqp = NULL;
	}

	if (req->resph) {
		req->resph(err, msg, req->arg);
		req->resph = NULL;
	}
}


static void timeout_handler(void *arg)
{
	struct http_req *req = arg;
	int err = ETIMEDOUT;

	if (req->srvc > 0) {

		err = req_connect(req);
		if (!err)
			return;
	}

	req_close(req, err, NULL);
	mem_deref(req);
}


static void estab_handler(void *arg)
{
	struct http_req *req = arg;
	int err;

	err = tcp_send(req->tc, req->mbreq);
	if (err) {
		req_close(req, err, NULL);
		mem_deref(req);
		return;
	}

	tmr_start(&req->tmr, RECV_TIMEOUT, timeout_handler, req);
}


static void recv_handler(struct mbuf *mb, void *arg)
{
	struct http_msg *msg = NULL;
	struct http_req *req = arg;
	size_t pos;
	int err;

	if (req->data) {
		if (req->datah)
			req->datah(mb, req->arg);
		return;
	}

	if (req->mb) {

		const size_t len = mbuf_get_left(mb);

		if ((mbuf_get_left(req->mb) + len) > BUFSIZE_MAX) {
			err = EOVERFLOW;
			goto out;
		}

		pos = req->mb->pos;
		req->mb->pos = req->mb->end;

		err = mbuf_write_mem(req->mb, mbuf_buf(mb), len);
		if (err)
			goto out;

		req->mb->pos = pos;
	}
	else {
		req->mb = mem_ref(mb);
	}

	pos = req->mb->pos;

	err = http_msg_decode(&msg, req->mb, false);
	if (err) {
		if (err == ENODATA) {
			req->mb->pos = pos;
			return;
		}
		goto out;
	}

	if (req->datah) {
		tmr_cancel(&req->tmr);
		req->data = true;
		if (req->resph)
			req->resph(0, msg, req->arg);
		mem_deref(msg);
		return;
	}

	if (mbuf_get_left(req->mb) < msg->clen) {
		req->mb->pos = pos;
		mem_deref(msg);
		return;
	}

	req->mb->end = req->mb->pos + msg->clen;

 out:
	req_close(req, err, msg);
	mem_deref(req);
	mem_deref(msg);
}


static void close_handler(int err, void *arg)
{
	struct http_req *req = arg;

	if (req->srvc > 0 && !req->data) {

		err = req_connect(req);
		if (!err)
			return;
	}

	req_close(req, err ? err : ECONNRESET, NULL);
	mem_deref(req);
}


static int req_connect(struct http_req *req)
{
	int err = EINVAL;

	while (req->srvc > 0) {

		--req->srvc;

		tmr_cancel(&req->tmr);
		req->sc = mem_deref(req->sc);
		req->tc = mem_deref(req->tc);
		req->mb = mem_deref(req->mb);

		err = tcp_connect(&req->tc, &req->srvv[req->srvc],
				  estab_handler, recv_handler, close_handler,
				  req);
		if (err)
			continue;

#ifdef USE_TLS
		if (req->secure) {

			err = tls_start_tcp(&req->sc, req->tls, req->tc, 0);
			if (err) {
				req->tc = mem_deref(req->tc);
				continue;
			}
		}
#endif
		tmr_start(&req->tmr, CONN_TIMEOUT, timeout_handler, req);
		break;
	}

	return err;
}


static bool rr_handler(struct dnsrr *rr, void *arg)
{
	struct http_req *req = arg;

	if (req->srvc >= ARRAY_SIZE(req->srvv))
		return true;

	switch (rr->type) {

	case DNS_TYPE_A:
		sa_set_in(&req->srvv[req->srvc++], rr->rdata.a.addr,
			  req->port);
		break;

	case DNS_TYPE_AAAA:
		sa_set_in6(&req->srvv[req->srvc++], rr->rdata.aaaa.addr,
			   req->port);
		break;
	}

	return false;
}


static void query_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			  struct list *authl, struct list *addl, void *arg)
{
	struct http_req *req = arg;
	(void)hdr;
	(void)authl;
	(void)addl;

	dns_rrlist_apply2(ansl, req->host, DNS_TYPE_A, DNS_TYPE_AAAA,
			  DNS_CLASS_IN, true, rr_handler, req);
	if (req->srvc == 0) {
		err = err ? err : EDESTADDRREQ;
		goto fail;
	}

	err = req_connect(req);
	if (err)
		goto fail;

	return;

 fail:
	req_close(req, err, NULL);
	mem_deref(req);
}


/**
 * Send an HTTP request
 *
 * @param reqp      Pointer to allocated HTTP request object
 * @param cli       HTTP Client
 * @param met       Request method
 * @param uri       Request URI
 * @param resph     Response handler
 * @param datah     Content handler (optional)
 * @param arg       Handler argument
 * @param fmt       Formatted HTTP headers and body (optional)
 *
 * @return 0 if success, otherwise errorcode
 */
int http_request(struct http_req **reqp, struct http_cli *cli, const char *met,
		 const char *uri, http_resp_h *resph, http_data_h *datah,
		 void *arg, const char *fmt, ...)
{
	struct pl scheme, host, port, path;
	struct http_req *req;
	uint16_t defport;
	bool secure;
	va_list ap;
	int err;

	if (!reqp || !cli || !met || !uri)
		return EINVAL;

	if (re_regex(uri, strlen(uri), "[a-z]+://[^:/]+[:]*[0-9]*[^]+",
		     &scheme, &host, NULL, &port, &path) || scheme.p != uri)
		return EINVAL;

	if (!pl_strcasecmp(&scheme, "http") ||
	    !pl_strcasecmp(&scheme, "ws")) {
		secure  = false;
		defport = 80;
	}
#ifdef USE_TLS
	else if (!pl_strcasecmp(&scheme, "https") ||
		 !pl_strcasecmp(&scheme, "wss")) {
		secure  = true;
		defport = 443;
	}
#endif
	else
		return ENOTSUP;

	req = mem_zalloc(sizeof(*req), req_destructor);
	if (!req)
		return ENOMEM;

	req->tls    = mem_ref(cli->tls);
	req->secure = secure;
	req->port   = pl_isset(&port) ? pl_u32(&port) : defport;
	req->resph  = resph;
	req->datah  = datah;
	req->arg    = arg;

	err = pl_strdup(&req->host, &host);
	if (err)
		goto out;

	req->mbreq = mbuf_alloc(1024);
	if (!req->mbreq) {
		err = ENOMEM;
		goto out;
	}

	err = mbuf_printf(req->mbreq,
			  "%s %r HTTP/1.1\r\n"
			  "Host: %r\r\n",
			  met, &path, &host);
	if (fmt) {
		va_start(ap, fmt);
		err |= mbuf_vprintf(req->mbreq, fmt, ap);
		va_end(ap);
	}
	else {
		err |= mbuf_write_str(req->mbreq, "\r\n");
	}
	if (err)
		goto out;

	req->mbreq->pos = 0;

	if (!sa_set_str(&req->srvv[0], req->host, req->port)) {

		req->srvc = 1;

		err = req_connect(req);
		if (err)
			goto out;
	}
	else {
		err = dnsc_query(&req->dq, cli->dnsc, req->host,
				 DNS_TYPE_A, DNS_CLASS_IN, true,
				 query_handler, req);
		if (err)
			goto out;
	}

 out:
	if (err)
		mem_deref(req);
	else {
		req->reqp = reqp;
		*reqp = req;
	}

	return err;
}


/**
 * Allocate an HTTP client instance
 *
 * @param clip      Pointer to allocated HTTP client
 * @param dnsc      DNS Client
 *
 * @return 0 if success, otherwise errorcode
 */
int http_client_alloc(struct http_cli **clip, struct dnsc *dnsc)
{
	struct http_cli *cli;
	int err;

	if (!clip || !dnsc)
		return EINVAL;

	cli = mem_zalloc(sizeof(*cli), cli_destructor);
	if (!cli)
		return ENOMEM;

#ifdef USE_TLS
	err = tls_alloc(&cli->tls, TLS_METHOD_SSLV23, NULL, NULL);
#else
	err = 0;
#endif
	if (err)
		goto out;

	cli->dnsc = mem_ref(dnsc);

 out:
	if (err)
		mem_deref(cli);
	else
		*clip = cli;

	return err;
}


struct tcp_conn *http_req_tcp(struct http_req *req)
{
	return req ? req->tc : NULL;
}


struct tls_conn *http_req_tls(struct http_req *req)
{
	return req ? req->sc : NULL;
}
