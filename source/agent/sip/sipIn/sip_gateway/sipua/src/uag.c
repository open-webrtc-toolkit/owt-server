/**
 * @file src/ua.c  User-Agent
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <baresip.h>
#include "core.h"


#define DEBUG_MODULE "uag"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

#define MAX_CALLS 10
extern int ua_call_alloc(struct call **callp, struct ua *ua,
                         enum vidmode vidmode, const struct sip_msg *msg,
                         struct call *xcall, const char *local_uri);

static void uag_destructor(void *arg)
{
	struct uag *uag = (struct uag *)arg;

	list_flush(&uag->ehl);

	uag_stop_all(uag);
	uag->evsock   = mem_deref(uag->evsock);
	uag->sock     = mem_deref(uag->sock);
	uag->lsnr     = mem_deref(uag->lsnr);
	uag->sip      = mem_deref(uag->sip);
	/*uag->sip      = mem_deref(uag->sip);*/
	uag->ua_cur   = mem_deref(uag->ua_cur);

#ifdef USE_TLS
	uag->tls = mem_deref(uag->tls);
#endif
}

/**
 * Stop all User-Agents
 *
 * @param forced True to force, otherwise false
 */
void uag_stop_all(struct uag *uag)
{
	sipsess_close_all(uag->sock);
	sip_close(uag->sip, true);
}


static int add_transp_af(struct uag *uag, const struct sa *laddr)
{
	struct sa local;
	int err = 0;

	sa_cpy(&local, laddr);
	sa_set_port(&local, 0);

	if (uag->use_udp)
		err |= sip_transp_add(uag->sip, SIP_TRANSP_UDP, &local);
	if (uag->use_tcp)
		err |= sip_transp_add(uag->sip, SIP_TRANSP_TCP, &local);
	if (err) {
		DEBUG_WARNING("SIP Transport failed: %m\n", err);
		return err;
	}

#ifdef USE_TLS
	if (uag->use_tls) {
		/* Build our SSL context*/
		if (!uag->tls) {
			const char *cert = NULL;

			if (str_isset(uag->cfg->cert)) {
				cert = uag->cfg->cert;
				info("SIP Certificate: %s\n", cert);
			}

			err = tls_alloc(&uag->tls, TLS_METHOD_SSLV23,
					cert, NULL);
			if (err) {
				DEBUG_WARNING("tls_alloc() failed: %m\n", err);
				return err;
			}
		}

		if (sa_isset(&local, SA_PORT))
			sa_set_port(&local, sa_port(&local) + 1);

		err = sip_transp_add(uag->sip, SIP_TRANSP_TLS, &local, uag->tls);
		if (err) {
			DEBUG_WARNING("SIP/TLS transport failed: %m\n", err);
			return err;
		}
	}
#endif

	return err;
}


/* This function is called when all SIP transactions are done */
static void exit_handler(void *arg)
{
	(void)arg;
	re_cancel();
}


static bool request_handler(const struct sip_msg *msg, void *arg)
{
	struct ua *ua;

	struct uag *uag = (struct uag *)arg;

	if (pl_strcmp(&msg->met, "OPTIONS"))
		return false;

	ua = uag_find(uag, &msg->uri.user);
	if (!ua) {
		(void)sip_treply(NULL, uag->sip, msg, 404, "Not Found");
		return true;
	}

	ua_handle_options(ua, msg);

	return true;
}


static bool require_handler(const struct sip_hdr *hdr,
			    const struct sip_msg *msg, void *arg)
{
	struct ua *ua = arg;
	bool supported = false;
	size_t i;
	(void)msg;

	for (i=0; i<ua->extensionc; i++) {

		if (!pl_casecmp(&hdr->val, &ua->extensionv[i])) {
			supported = true;
			break;
		}
	}

	return !supported;
}


/* Handle incoming calls */
static void sipsess_conn_handler(const struct sip_msg *msg, void *arg)
{
	const struct sip_hdr *hdr;
	struct uag *uag;
	struct ua *ua;
	struct call *call = NULL;
	char str[256], to_uri[256];
	int err;

	uag = (struct uag *)arg;

	ua = uag_find(uag, &msg->uri.user);
	if (!ua) {
		DEBUG_WARNING("%r: UA not found: %r\n",
			      &msg->from.auri, &msg->uri.user);
		(void)sip_treply(NULL, uag->sip, msg, 404, "Not Found");
		return;
	}

	/* handle multiple calls */
	/* remove max calls limits here */
	/*
	if (list_count(&ua->calls) + 1 > MAX_CALLS) {
		info("ua: rejected call from %r (maximum %d calls)\n",
		     &msg->from.auri, MAX_CALLS);
		(void)sip_treply(NULL, uag->sip, msg, 486, "Busy Here");
		return;
	}
	*/

	/* Handle Require: header, check for any required extensions */
	hdr = sip_msg_hdr_apply(msg, true, SIP_HDR_REQUIRE,
				require_handler, ua);
	if (hdr) {
		info("ua: call from %r rejected with 420"
			     " -- option-tag '%r' not supported\n",
			     &msg->from.auri, &hdr->val);

		(void)sip_treplyf(NULL, NULL, uag->sip, msg, false,
				  420, "Bad Extension",
				  "Unsupported: %r\r\n"
				  "Content-Length: 0\r\n\r\n",
				  &hdr->val);
		return;
	}

	(void)pl_strcpy(&msg->to.auri, to_uri, sizeof(to_uri));

	err = ua_call_alloc(&call, ua, VIDMODE_ON, msg, NULL, to_uri);
	if (err) {
		DEBUG_WARNING("call_alloc: %m\n", err);
		goto error;
	}

	err = call_accept(call, uag->sock, msg);
	if (err)
		goto error;

	return;

 error:
	mem_deref(call);
	(void)re_snprintf(str, sizeof(str), "Error (%m)", err);
	(void)sip_treply(NULL, uag->sip, msg, 500, str);
}

/*
static void net_change_handler(void *arg)
{
	struct uag *uag = (struct uag *)arg;

	info("IP-address changed: %j\n", net_laddr_af(AF_INET));

	(void)uag_reset_transp(uag, true, true);
}
*/



static int uag_add_transp(struct uag *uag)
{
	int err = 0;

	if (!uag->prefer_ipv6) {
		if (sa_isset(net_laddr_af(AF_INET), SA_ADDR))
			err |= add_transp_af(uag, net_laddr_af(AF_INET));
	}

#if HAVE_INET6
	if (sa_isset(net_laddr_af(AF_INET6), SA_ADDR))
		err |= add_transp_af(uag, net_laddr_af(AF_INET6));
#endif

    if (err) {
    	warning("uag_add_transp FAILED.\n");
    }
	return err;
}

/**
 * Reset the SIP transports for all User-Agents
 *
 * @param reg      True to reset registration
 * @param reinvite True to update active calls
 *
 * @return 0 if success, otherwise errorcode
 */
int uag_reset_transp(struct uag *uag, bool reg, bool reinvite)
{
	struct ua *ua;
	int err;

	/* Update SIP transports */
	sip_transp_flush(uag->sip);

	(void)net_check();
	err = uag_add_transp(uag);
	if (err)
		return err;
    
    /* Re-REGISTER all User-Agents */
    ua = uag->ua_cur;
	if (reg && ua->acc->regint) {
		err |= ua_register(ua);
	}

	/* update all active calls */
	if (reinvite) {
		struct le *lec;

		for (lec = ua->calls.head; lec; lec = lec->next) {
			struct call *call = lec->data;

			err |= call_reset_transp(call);
		}
	}


	return err;
}


/**
 * Initialise the User-Agents
 *
 * @param software    SIP User-Agent string
 * @param udp         Enable UDP transport
 * @param tcp         Enable TCP transport
 * @param tls         Enable TLS transport
 * @param prefer_ipv6 Prefer IPv6 flag
 *
 * @return 0 if success, otherwise errorcode
 */
int uag_alloc(struct uag **uagp, const char *software, void *ep, bool udp, bool tcp, bool tls, bool prefer_ipv6)
{
	struct uag *uag = NULL;
	struct config *cfg = conf_config();
	uint32_t bsize;
	int err;

	uag = mem_zalloc(sizeof(struct uag), uag_destructor);
	if (!uag) {
		err = -1;
		goto out;
	}

    uag->ep = ep;
	uag->cfg = &cfg->sip;
	uag->ehl.head = NULL; uag->ehl.tail = NULL;
	uag->sip = NULL;
	uag->lsnr = NULL;
	uag->sock = NULL;
	uag->evsock = NULL;
	uag->ua_cur = NULL;
	uag->use_udp = udp;
	uag->use_tcp = tcp;
#ifdef USE_TLS
	uag->use_tls = tls;
#endif
	uag->prefer_ipv6 = prefer_ipv6;

	bsize = cfg->sip.trans_bsize;

	err = sip_alloc(&uag->sip, net_dnsc(), bsize, bsize, bsize,
			software, exit_handler, NULL);
	if (err) {
		DEBUG_WARNING("sip stack failed: %m\n", err);
		goto out;
	}

	err = uag_add_transp(uag);
	if (err)
		goto out;

	err = sip_listen(&uag->lsnr, uag->sip, true, request_handler, uag);
	if (err)
		goto out;

	err = sipsess_listen(&uag->sock, uag->sip, bsize,
			     sipsess_conn_handler, uag);
	if (err)
		goto out;

	err = sipevent_listen(&uag->evsock, uag->sip, bsize, bsize, NULL, NULL);
	if (err)
		goto out;

 out:
	if (err) {
		DEBUG_WARNING("init failed (%m)\n", err);
		mem_deref(uag);
	}
	*uagp = uag;
	return err;
}


/**
 * Print the SIP Status for all User-Agents
 *
 * @param pf     Print handler for debug output
 * @param unused Unused parameter
 *
 * @return 0 if success, otherwise errorcode
 */
int uag_print_sip_status(const struct uag *uag, struct re_printf *pf, void *unused)
{
	(void)unused;
	return sip_debug(pf, uag->sip);
}


/**
 * Get the name of the User-Agent event
 *
 * @param ev User-Agent event
 *
 * @return Name of the event
 */
const char *uag_event_str(enum ua_event ev)
{
	switch (ev) {

	case UA_EVENT_REGISTERING:      return "REGISTERING";
	case UA_EVENT_REGISTER_OK:      return "REGISTER_OK";
	case UA_EVENT_REGISTER_FAIL:    return "REGISTER_FAIL";
	case UA_EVENT_UNREGISTERING:    return "UNREGISTERING";
	case UA_EVENT_CALL_INCOMING:    return "CALL_INCOMING";
	case UA_EVENT_CALL_RINGING:     return "CALL_RINGING";
	case UA_EVENT_CALL_PROGRESS:    return "CALL_PROGRESS";
	case UA_EVENT_CALL_ESTABLISHED: return "CALL_ESTABLISHED";
	case UA_EVENT_CALL_CLOSED:      return "CALL_CLOSED";
	default: return "?";
	}
}


/**
 * Find the correct UA from the contact user
 *
 * @param cuser Contact username
 *
 * @return Matching UA if found, NULL if not found
 */
struct ua *uag_find(const struct uag *uag, const struct pl *cuser)
{
	
	struct ua *ua = uag->ua_cur;

	if (0 == pl_strcasecmp(cuser, ua->cuser))
		return ua;
	else if (0 == pl_casecmp(cuser, &ua->acc->luri.user))
		return ua;
    else
    	return NULL;

	return NULL;
}


/**
 * Find a User-Agent (UA) from an Address-of-Record (AOR)
 *
 * @param aor Address-of-Record string
 *
 * @return User-Agent (UA) if found, otherwise NULL
 */
struct ua *uag_find_aor(const struct uag *uag, const char *aor)
{
	struct ua *ua = uag->ua_cur;

	if (str_isset(aor) && str_cmp(ua->acc->aor, aor))
		return ua;
	else
		return NULL;
}


/**
 * Find a User-Agent (UA) which has certain address parameter and/or value
 *
 * @param name  SIP Address parameter name
 * @param value SIP Address parameter value (optional)
 *
 * @return User-Agent (UA) if found, otherwise NULL
 */
struct ua *uag_find_param(const struct uag *uag, const char *name, const char *value)
{
	struct ua *ua = uag->ua_cur;
	struct sip_addr *laddr = account_laddr(ua->acc);
	struct pl val;

	if (value) {
		if (0 == msg_param_decode(&laddr->params, name, &val)
		    &&
		    0 == pl_strcasecmp(&val, value)) {
			return ua;
		}
	}
	else {
		if (0 == msg_param_exists(&laddr->params, name, &val))
			return ua;
	}


	return NULL;
}

/**
 * Return list of methods supported by the UA
 *
 * @return String of supported methods
 */
const char *uag_allowed_methods(void)
{
	return "INVITE,ACK,BYE,CANCEL,OPTIONS,REFER,NOTIFY,SUBSCRIBE,INFO";
}


static void eh_destructor(void *arg)
{
	struct ua_eh *eh = arg;
	list_unlink(&eh->le);
}


int uag_event_register(struct uag *uag, ua_event_h *h, void *arg)
{
	struct ua_eh *eh;

	if (!h)
		return EINVAL;

	eh = mem_zalloc(sizeof(*eh), eh_destructor);
	if (!eh)
		return ENOMEM;

	eh->h = h;
	eh->arg = arg;

	list_append(&uag->ehl, &eh->le, eh);

	return 0;
}


void uag_event_unregister(struct uag *uag, ua_event_h *h)
{
	struct le *le;

	for (le = uag->ehl.head; le; le = le->next) {

		struct ua_eh *eh = le->data;

		if (eh->h == h) {
			mem_deref(eh);
			break;
		}
	}
}


void uag_current_set(struct uag *uag, struct ua *ua)
{
	uag->ua_cur = ua;
}


struct ua *uag_current(struct uag *uag)
{
	return uag->ua_cur;
}
