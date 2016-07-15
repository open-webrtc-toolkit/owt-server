/**
 * @file sip/request.c  SIP Request
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_dns.h>
#include <re_uri.h>
#include <re_sys.h>
#include <re_udp.h>
#include <re_msg.h>
#include <re_sip.h>
#include "sip.h"


struct sip_request {
	struct le le;
	struct list cachel;
	struct list addrl;
	struct list srvl;
	struct sip_request **reqp;
	struct sip_ctrans *ct;
	struct dns_query *dnsq;
	struct dns_query *dnsq2;
	struct sip *sip;
	char *met;
	char *uri;
	char *host;
	struct mbuf *mb;
	sip_send_h *sendh;
	sip_resp_h *resph;
	void *arg;
	enum sip_transp tp;
	bool tp_selected;
	bool stateful;
	bool canceled;
	bool provrecv;
	uint16_t port;
};


static int  request_next(struct sip_request *req);
static bool rr_append_handler(struct dnsrr *rr, void *arg);
static void srv_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			struct list *authl, struct list *addl, void *arg);
static int  srv_lookup(struct sip_request *req, const char *domain);
static int  addr_lookup(struct sip_request *req, const char *name);


static int str_ldup(char **dst, const char *src, int len)
{
	struct pl pl;

	pl.p = src;
	pl.l = len < 0 ? str_len(src) : (size_t)len;

	return pl_strdup(dst, &pl);
}


static void destructor(void *arg)
{
	struct sip_request *req = arg;

	if (req->reqp && req->stateful) {
		/* user does deref before request has completed */
		*req->reqp = NULL;
		req->reqp  = NULL;
		req->sendh = NULL;
		req->resph = NULL;
		sip_request_cancel(mem_ref(req));
		return;
	}

	list_flush(&req->cachel);
	list_flush(&req->addrl);
	list_flush(&req->srvl);
	list_unlink(&req->le);
	mem_deref(req->dnsq);
	mem_deref(req->dnsq2);
	mem_deref(req->ct);
	mem_deref(req->met);
	mem_deref(req->uri);
	mem_deref(req->host);
	mem_deref(req->mb);
}


static void terminate(struct sip_request *req, int err,
		      const struct sip_msg *msg)
{
	if (req->reqp) {
		*req->reqp = NULL;
		req->reqp = NULL;
	}

	list_unlink(&req->le);
	req->sendh = NULL;

	if (req->resph) {
		req->resph(err, msg, req->arg);
		req->resph = NULL;
	}
}


static bool close_handler(struct le *le, void *arg)
{
	struct sip_request *req = le->data;
	(void)arg;

	req->dnsq  = mem_deref(req->dnsq);
	req->dnsq2 = mem_deref(req->dnsq2);
	req->ct    = mem_deref(req->ct);

	terminate(req, ECONNABORTED, NULL);
	mem_deref(req);

	return false;
}


static void response_handler(int err, const struct sip_msg *msg, void *arg)
{
	struct sip_request *req = arg;

	if (msg && msg->scode < 200) {
		if (!req->provrecv) {
			req->provrecv = true;
			if (req->canceled)
				(void)sip_ctrans_cancel(req->ct);
		}

		if (req->resph)
			req->resph(err, msg, req->arg);

		return;
	}

	req->ct = NULL;

	if (!req->canceled && (err || msg->scode == 503) &&
	    (req->addrl.head || req->srvl.head)) {

		err = request_next(req);
		if (!err)
			return;
	}

	terminate(req, err, msg);
	mem_deref(req);
}


static int request(struct sip_request *req, enum sip_transp tp,
		   const struct sa *dst)
{
	struct mbuf *mb = NULL;
	char *branch = NULL;
	int err = ENOMEM;
	struct sa laddr;

	req->provrecv = false;

	branch = mem_alloc(24, NULL);
	mb = mbuf_alloc(1024);

	if (!branch || !mb)
		goto out;

	(void)re_snprintf(branch, 24, "z9hG4bK%016llx", rand_u64());

	err = sip_transp_laddr(req->sip, &laddr, tp, dst);
	if (err)
		goto out;

	err  = mbuf_printf(mb, "%s %s SIP/2.0\r\n", req->met, req->uri);
	err |= mbuf_printf(mb, "Via: SIP/2.0/%s %J;branch=%s;rport\r\n",
			   sip_transp_name(tp), &laddr, branch);
	err |= req->sendh ? req->sendh(tp, &laddr, dst, mb, req->arg) : 0;
	err |= mbuf_write_mem(mb, mbuf_buf(req->mb), mbuf_get_left(req->mb));
	if (err)
		goto out;

	mb->pos = 0;

	if (!req->stateful)
		err = sip_send(req->sip, NULL, tp, dst, mb);
	else
		err = sip_ctrans_request(&req->ct, req->sip, tp, dst, req->met,
					 branch, mb, response_handler, req);
	if (err)
		goto out;

 out:
	mem_deref(branch);
	mem_deref(mb);

	return err;
}


static int request_next(struct sip_request *req)
{
	struct dnsrr *rr;
	struct sa dst;
	int err;

 again:
	rr = list_ledata(req->addrl.head);
	if (!rr) {
		rr = list_ledata(req->srvl.head);
		if (!rr)
			return ENOENT;

		req->port = rr->rdata.srv.port;

		dns_rrlist_apply2(&req->cachel, rr->rdata.srv.target,
				  DNS_TYPE_A, DNS_TYPE_AAAA, DNS_CLASS_IN,
				  true, rr_append_handler, &req->addrl);

		list_unlink(&rr->le);

		if (req->addrl.head) {
			mem_deref(rr);
			goto again;
		}

		err = addr_lookup(req, rr->rdata.srv.target);
		mem_deref(rr);

		return err;
	}

	switch (rr->type) {

	case DNS_TYPE_A:
		sa_set_in(&dst, rr->rdata.a.addr, req->port);
		break;

	case DNS_TYPE_AAAA:
		sa_set_in6(&dst, rr->rdata.aaaa.addr, req->port);
		break;

	default:
		return EINVAL;
	}

	list_unlink(&rr->le);
	mem_deref(rr);

	err = request(req, req->tp, &dst);
	if (err) {
		if (req->addrl.head || req->srvl.head)
			goto again;
	}

	return err;
}


static bool transp_next(struct sip *sip, enum sip_transp *tp)
{
	enum sip_transp i;

	for (i=(enum sip_transp)(*tp+1); i<SIP_TRANSPC; i++) {

		if (!sip_transp_supported(sip, i, AF_UNSPEC))
			continue;

		*tp = i;
		return true;
	}

	return false;
}


static bool transp_next_srv(struct sip *sip, enum sip_transp *tp)
{
	enum sip_transp i;

	for (i=(enum sip_transp)(*tp-1); i>SIP_TRANSP_NONE; i--) {

		if (!sip_transp_supported(sip, i, AF_UNSPEC))
			continue;

		*tp = i;
		return true;
	}

	return false;
}


static bool rr_append_handler(struct dnsrr *rr, void *arg)
{
	struct list *lst = arg;

	switch (rr->type) {

	case DNS_TYPE_A:
	case DNS_TYPE_AAAA:
	case DNS_TYPE_SRV:
		if (rr->le.list)
			break;

		list_append(lst, &rr->le, mem_ref(rr));
		break;
	}

	return false;
}


static bool rr_cache_handler(struct dnsrr *rr, void *arg)
{
	struct sip_request *req = arg;

	switch (rr->type) {

	case DNS_TYPE_A:
		if (!sip_transp_supported(req->sip, req->tp, AF_INET))
			break;

		list_unlink(&rr->le_priv);
		list_append(&req->cachel, &rr->le_priv, rr);
		break;

#ifdef HAVE_INET6
	case DNS_TYPE_AAAA:
		if (!sip_transp_supported(req->sip, req->tp, AF_INET6))
			break;

		list_unlink(&rr->le_priv);
		list_append(&req->cachel, &rr->le_priv, rr);
		break;
#endif

	case DNS_TYPE_CNAME:
		list_unlink(&rr->le_priv);
		list_append(&req->cachel, &rr->le_priv, rr);
		break;
	}

	return false;
}


static bool rr_naptr_handler(struct dnsrr *rr, void *arg)
{
	struct sip_request *req = arg;
	enum sip_transp tp;

	if (rr->type != DNS_TYPE_NAPTR)
		return false;

	if (!str_casecmp(rr->rdata.naptr.services, "SIP+D2U"))
		tp = SIP_TRANSP_UDP;
	else if (!str_casecmp(rr->rdata.naptr.services, "SIP+D2T"))
		tp = SIP_TRANSP_TCP;
	else if (!str_casecmp(rr->rdata.naptr.services, "SIPS+D2T"))
		tp = SIP_TRANSP_TLS;
	else
		return false;

	if (!sip_transp_supported(req->sip, tp, AF_UNSPEC))
		return false;

	req->tp = tp;
	req->tp_selected = true;

	return true;
}


static void naptr_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			  struct list *authl, struct list *addl, void *arg)
{
	struct sip_request *req = arg;
	struct dnsrr *rr;
	(void)hdr;
	(void)authl;

	dns_rrlist_sort(ansl, DNS_TYPE_NAPTR);

	rr = dns_rrlist_apply(ansl, NULL, DNS_TYPE_NAPTR, DNS_CLASS_IN, false,
			      rr_naptr_handler, req);
	if (!rr) {
		req->tp = SIP_TRANSPC;
		if (!transp_next_srv(req->sip, &req->tp)) {
			err = EPROTONOSUPPORT;
			goto fail;
		}

		err = srv_lookup(req, req->host);
		if (err)
			goto fail;

		return;
	}

	dns_rrlist_sort(addl, DNS_TYPE_SRV);

	dns_rrlist_apply(addl, rr->rdata.naptr.replace, DNS_TYPE_SRV,
			 DNS_CLASS_IN, true, rr_append_handler, &req->srvl);

	if (!req->srvl.head) {
		err = dnsc_query(&req->dnsq, req->sip->dnsc,
				 rr->rdata.naptr.replace, DNS_TYPE_SRV,
				 DNS_CLASS_IN, true, srv_handler, req);
		if (err)
			goto fail;

		return;
	}

	dns_rrlist_apply(addl, NULL, DNS_QTYPE_ANY, DNS_CLASS_IN, false,
			 rr_cache_handler, req);

	err = request_next(req);
	if (err)
		goto fail;

	if (!req->stateful) {
		req->resph = NULL;
		terminate(req, 0, NULL);
		mem_deref(req);
	}

	return;

 fail:
	terminate(req, err, NULL);
	mem_deref(req);
}


static void srv_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			 struct list *authl, struct list *addl, void *arg)
{
	struct sip_request *req = arg;
	(void)hdr;
	(void)authl;

	dns_rrlist_sort(ansl, DNS_TYPE_SRV);

	dns_rrlist_apply(ansl, NULL, DNS_TYPE_SRV, DNS_CLASS_IN, false,
			 rr_append_handler, &req->srvl);

	if (!req->srvl.head) {
		if (!req->tp_selected) {
			if (transp_next_srv(req->sip, &req->tp)) {

				err = srv_lookup(req, req->host);
				if (err)
					goto fail;

				return;
			}

			req->tp = SIP_TRANSP_NONE;
			if (!transp_next(req->sip, &req->tp)) {
				err = EPROTONOSUPPORT;
				goto fail;
			}
		}

		req->port = sip_transp_port(req->tp, 0);

		err = addr_lookup(req, req->host);
		if (err)
			goto fail;

		return;
	}

	dns_rrlist_apply(addl, NULL, DNS_QTYPE_ANY, DNS_CLASS_IN, false,
			 rr_cache_handler, req);

	err = request_next(req);
	if (err)
		goto fail;

	if (!req->stateful) {
		req->resph = NULL;
		terminate(req, 0, NULL);
		mem_deref(req);
	}

	return;

 fail:
	terminate(req, err, NULL);
	mem_deref(req);
}


static void addr_handler(int err, const struct dnshdr *hdr, struct list *ansl,
			 struct list *authl, struct list *addl, void *arg)
{
	struct sip_request *req = arg;
	(void)hdr;
	(void)authl;
	(void)addl;

	dns_rrlist_apply2(ansl, NULL, DNS_TYPE_A, DNS_TYPE_AAAA, DNS_CLASS_IN,
			  false, rr_append_handler, &req->addrl);

	/* wait for other (A/AAAA) query to complete */
	if (req->dnsq || req->dnsq2)
		return;

	if (!req->addrl.head && !req->srvl.head) {
		err = err ? err : EDESTADDRREQ;
		goto fail;
	}

	err = request_next(req);
	if (err)
		goto fail;

	if (!req->stateful) {
		req->resph = NULL;
		terminate(req, 0, NULL);
		mem_deref(req);
	}

	return;

 fail:
	terminate(req, err, NULL);
	mem_deref(req);
}


static int srv_lookup(struct sip_request *req, const char *domain)
{
	char name[256];

	if (re_snprintf(name, sizeof(name), "%s.%s",
			sip_transp_srvid(req->tp), domain) < 0)
		return ENOMEM;

	return dnsc_query(&req->dnsq, req->sip->dnsc, name, DNS_TYPE_SRV,
			  DNS_CLASS_IN, true, srv_handler, req);
}


static int addr_lookup(struct sip_request *req, const char *name)
{
	int err;

	if (sip_transp_supported(req->sip, req->tp, AF_INET)) {

		err = dnsc_query(&req->dnsq, req->sip->dnsc, name,
				 DNS_TYPE_A, DNS_CLASS_IN, true,
				 addr_handler, req);
		if (err)
			return err;
	}

#ifdef HAVE_INET6
	if (sip_transp_supported(req->sip, req->tp, AF_INET6)) {

		err = dnsc_query(&req->dnsq2, req->sip->dnsc, name,
				 DNS_TYPE_AAAA, DNS_CLASS_IN, true,
				 addr_handler, req);
		if (err)
			return err;
	}
#endif

	if (!req->dnsq && !req->dnsq2)
		return EPROTONOSUPPORT;

	return 0;
}


/**
 * Send a SIP request
 *
 * @param reqp     Pointer to allocated SIP request object
 * @param sip      SIP Stack
 * @param stateful Stateful client transaction
 * @param met      SIP Method string
 * @param metl     Length of SIP Method string
 * @param uri      Request URI
 * @param uril     Length of Request URI string
 * @param route    Next hop route URI
 * @param mb       Buffer containing SIP request
 * @param sendh    Send handler
 * @param resph    Response handler
 * @param arg      Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_request(struct sip_request **reqp, struct sip *sip, bool stateful,
		const char *met, int metl, const char *uri, int uril,
		const struct uri *route, struct mbuf *mb,
		sip_send_h *sendh, sip_resp_h *resph, void *arg)
{
	struct sip_request *req;
	struct sa dst;
	struct pl pl;
	int err;

	if (!sip || !met || !uri || !route || !mb)
		return EINVAL;

	if (pl_strcasecmp(&route->scheme, "sip"))
		return ENOSYS;

	req = mem_zalloc(sizeof(*req), destructor);
	if (!req)
		return ENOMEM;

	list_append(&sip->reql, &req->le, req);

	err = str_ldup(&req->met, met, metl);
	if (err)
		goto out;

	err = str_ldup(&req->uri, uri, uril);
	if (err)
		goto out;

	if (msg_param_decode(&route->params, "maddr", &pl))
		pl = route->host;

	err = pl_strdup(&req->host, &pl);
	if (err)
		goto out;

	req->stateful = stateful;
	req->mb    = mem_ref(mb);
	req->sip   = sip;
	req->sendh = sendh;
	req->resph = resph;
	req->arg   = arg;

	if (!msg_param_decode(&route->params, "transport", &pl)) {

		if (!pl_strcasecmp(&pl, "udp"))
			req->tp = SIP_TRANSP_UDP;
		else if (!pl_strcasecmp(&pl, "tcp"))
			req->tp = SIP_TRANSP_TCP;
		else if (!pl_strcasecmp(&pl, "tls"))
			req->tp = SIP_TRANSP_TLS;
		else {
			err = EPROTONOSUPPORT;
			goto out;
		}

		if (!sip_transp_supported(sip, req->tp, AF_UNSPEC)) {
			err = EPROTONOSUPPORT;
			goto out;
		}

		req->tp_selected = true;
	}
	else {
		req->tp = SIP_TRANSP_NONE;
		if (!transp_next(sip, &req->tp)) {
			err = EPROTONOSUPPORT;
			goto out;
		}

		req->tp_selected = false;
	}

	if (!sa_set_str(&dst, req->host,
			sip_transp_port(req->tp, route->port))) {

		err = request(req, req->tp, &dst);
		if (!req->stateful) {
			mem_deref(req);
			return err;
		}
	}
	else if (route->port) {

		req->port = sip_transp_port(req->tp, route->port);
		err = addr_lookup(req, req->host);
	}
	else if (req->tp_selected) {

		err = srv_lookup(req, req->host);
	}
	else {
	        err = dnsc_query(&req->dnsq, sip->dnsc, req->host,
				 DNS_TYPE_NAPTR, DNS_CLASS_IN, true,
				 naptr_handler, req);
	}

 out:
	if (err)
		mem_deref(req);
	else if (reqp) {
		req->reqp = reqp;
		*reqp = req;
	}

	return err;
}


/**
 * Send a SIP request with formatted arguments
 *
 * @param reqp     Pointer to allocated SIP request object
 * @param sip      SIP Stack
 * @param stateful Stateful client transaction
 * @param met      Null-terminated SIP Method string
 * @param uri      Null-terminated Request URI string
 * @param route    Next hop route URI (optional)
 * @param auth     SIP authentication state
 * @param sendh    Send handler
 * @param resph    Response handler
 * @param arg      Handler argument
 * @param fmt      Formatted SIP headers and body
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_requestf(struct sip_request **reqp, struct sip *sip, bool stateful,
		 const char *met, const char *uri, const struct uri *route,
		 struct sip_auth *auth, sip_send_h *sendh, sip_resp_h *resph,
		 void *arg, const char *fmt, ...)
{
	struct uri lroute;
	struct mbuf *mb;
	va_list ap;
	int err;

	if (!sip || !met || !uri || !fmt)
		return EINVAL;

	if (!route) {
		struct pl uripl;

		pl_set_str(&uripl, uri);

		err = uri_decode(&lroute, &uripl);
		if (err)
			return err;

		route = &lroute;
	}

	mb = mbuf_alloc(2048);
	if (!mb)
		return ENOMEM;

	err = mbuf_write_str(mb, "Max-Forwards: 70\r\n");

	if (auth)
		err |= sip_auth_encode(mb, auth, met, uri);

	if (err)
		goto out;

	va_start(ap, fmt);
	err = mbuf_vprintf(mb, fmt, ap);
	va_end(ap);

	if (err)
		goto out;

	mb->pos = 0;

	err = sip_request(reqp, sip, stateful, met, -1, uri, -1, route, mb,
			  sendh, resph, arg);
	if (err)
		goto out;

 out:
	mem_deref(mb);

	return err;
}


/**
 * Send a SIP dialog request with formatted arguments
 *
 * @param reqp     Pointer to allocated SIP request object
 * @param sip      SIP Stack
 * @param stateful Stateful client transaction
 * @param met      Null-terminated SIP Method string
 * @param dlg      SIP Dialog state
 * @param cseq     CSeq number
 * @param auth     SIP authentication state
 * @param sendh    Send handler
 * @param resph    Response handler
 * @param arg      Handler argument
 * @param fmt      Formatted SIP headers and body
 *
 * @return 0 if success, otherwise errorcode
 */
int sip_drequestf(struct sip_request **reqp, struct sip *sip, bool stateful,
		  const char *met, struct sip_dialog *dlg, uint32_t cseq,
		  struct sip_auth *auth, sip_send_h *sendh, sip_resp_h *resph,
		  void *arg, const char *fmt, ...)
{
	struct mbuf *mb;
	va_list ap;
	int err;

	if (!sip || !met || !dlg || !fmt)
		return EINVAL;

	mb = mbuf_alloc(2048);
	if (!mb)
		return ENOMEM;

	err = mbuf_write_str(mb, "Max-Forwards: 70\r\n");

	if (auth)
		err |= sip_auth_encode(mb, auth, met, sip_dialog_uri(dlg));

	err |= sip_dialog_encode(mb, dlg, cseq, met);

	if (sip->software)
		err |= mbuf_printf(mb, "User-Agent: %s\r\n", sip->software);

	if (err)
		goto out;

	va_start(ap, fmt);
	err = mbuf_vprintf(mb, fmt, ap);
	va_end(ap);

	if (err)
		goto out;

	mb->pos = 0;

	err = sip_request(reqp, sip, stateful, met, -1, sip_dialog_uri(dlg),
			  -1, sip_dialog_route(dlg), mb, sendh, resph, arg);
	if (err)
		goto out;

 out:
	mem_deref(mb);

	return err;
}


/**
 * Cancel a pending SIP Request
 *
 * @param req SIP Request
 */
void sip_request_cancel(struct sip_request *req)
{
	if (!req || req->canceled)
		return;

	req->canceled = true;

	if (!req->provrecv)
		return;

	(void)sip_ctrans_cancel(req->ct);
}


void sip_request_close(struct sip *sip)
{
	if (!sip)
		return;

	list_apply(&sip->reql, true, close_handler, NULL);
}


/**
 * Check if a SIP request loops
 *
 * @param ls    Loop state
 * @param scode Status code from SIP response
 *
 * @return True if loops, otherwise false
 */
bool sip_request_loops(struct sip_loopstate *ls, uint16_t scode)
{
	bool loop = false;

	if (!ls)
		return false;

	if (scode < 200) {
		return false;
	}
	else if (scode < 300) {
		ls->failc = 0;
	}
	else if (scode < 400) {
		loop = (++ls->failc >= 16);
	}
	else {
		switch (scode) {

		default:
			if (ls->last_scode == scode)
				loop = true;
			/*@fallthrough@*/
		case 401:
		case 407:
		case 491:
			if (++ls->failc >= 16)
				loop = true;
			break;
		}
	}

	ls->last_scode = scode;

	return loop;
}


/**
 * Reset the loop state
 *
 * @param ls Loop state
 */
void sip_loopstate_reset(struct sip_loopstate *ls)
{
	if (!ls)
		return;

	ls->last_scode = 0;
	ls->failc = 0;
}
