/**
 * @file src/ua.c  User-Agent
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <baresip.h>
#include "core.h"
#include <ctype.h>


enum {
	MAX_CALLS       =   20 
};

void ua_printf(const struct ua *ua, const char *fmt, ...)
{
	va_list ap;

	if (!ua)
		return;

	va_start(ap, fmt);
	info("%r@%r: %v", &ua->acc->luri.user, &ua->acc->luri.host, fmt, &ap);
	va_end(ap);
}

void ua_event(struct ua *ua, enum ua_event ev, struct call *call,
	      const char *fmt, ...)
{
	struct uag *uag;
	struct le *le;
	char buf[256];
	va_list ap;

	if (!ua)
		return;

        uag = ua->owner;
	va_start(ap, fmt);
	(void)re_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	/* send event to all clients */
	if (!uag) {
		for (le = uag->ehl.head; le; le = le->next) {

			struct ua_eh *eh = le->data;

			eh->h(ua, ev, call, buf, eh->arg);
		}
	}
}

/**
 * Get the contact user of a User-Agent (UA)
 *
 * @param ua User-Agent
 *
 * @return Contact user
 */
const char *ua_cuser(const struct ua *ua)
{
	return ua ? ua->cuser : NULL;
}


/**
 * Start registration of a User-Agent
 *
 * @param ua User-Agent
 *
 * @return 0 if success, otherwise errorcode
 */
int ua_register(struct ua *ua)
{
	struct account *acc;
	struct le *le;
	struct uri uri;
	char reg_uri[64];
	char params[256] = "";
	unsigned i;
	int err;

	if (!ua)
		return EINVAL;

	acc = ua->acc;
	uri = ua->acc->luri;
	uri.user = uri.password = pl_null;
	if (re_snprintf(reg_uri, sizeof(reg_uri), "%H", uri_encode, &uri) < 0)
		return ENOMEM;

	/* TODO intel webrtc
	if (uag.cfg && str_isset(uag.cfg->uuid)) {
		if (re_snprintf(params, sizeof(params),
				";+sip.instance=\"<urn:uuid:%s>\"",
				uag.cfg->uuid) < 0)
			return ENOMEM;
	}
	*/

	if (acc->regq) {
		if (re_snprintf(&params[strlen(params)],
				sizeof(params) - strlen(params),
				";q=%s", acc->regq) < 0)
			return ENOMEM;
	}

	if (acc->mnat && acc->mnat->ftag) {
		if (re_snprintf(&params[strlen(params)],
				sizeof(params) - strlen(params),
				";%s", acc->mnat->ftag) < 0)
			return ENOMEM;
	}

	ua_event(ua, UA_EVENT_REGISTERING, NULL, NULL);

	for (le = ua->regl.head, i=0; le; le = le->next, i++) {
		struct reg *reg = le->data;

		err = reg_register(reg, reg_uri, params,
				   acc->regint, acc->outbound[i]);
		if (err) {
			warning("ua: SIP register failed: %m\n", err);
			return err;
		}
	}

	return 0;
}


/**
 * Unregister all Register clients of a User-Agent
 *
 * @param ua User-Agent
 */
void ua_unregister(struct ua *ua)
{
	struct le *le;

	if (!ua)
		return;

	if (!list_isempty(&ua->regl))
		ua_event(ua, UA_EVENT_UNREGISTERING, NULL, NULL);

	for (le = ua->regl.head; le; le = le->next) {
		struct reg *reg = le->data;

		reg_unregister(reg);
	}
}


bool ua_isregistered(const struct ua *ua)
{
	struct le *le;

	if (!ua)
		return false;

	for (le = ua->regl.head; le; le = le->next) {

		const struct reg *reg = le->data;

		/* it is enough if one of the registrations work */
		if (reg_isok(reg))
			return true;
	}

	return false;
}


static struct call *ua_find_call_onhold(const struct ua *ua)
{
	struct le *le;

	if (!ua)
		return NULL;

	for (le = ua->calls.tail; le; le = le->prev) {

		struct call *call = le->data;

		if (call_is_onhold(call))
			return call;
	}

	return NULL;
}


static void resume_call(struct ua *ua)
{
	struct call *call;

	call = ua_find_call_onhold(ua);
	if (call) {
		ua_printf(ua, "resuming previous call with '%s'\n",
			  call_peeruri(call));
		call_hold(call, false);
	}
}

extern int ep_incoming_call(void *gateway, unsigned char audio, unsigned char video, const char *callerURI);
extern void ep_peer_ringing(void *gateway, const char *peer);
extern void ep_call_closed(void *gateway, const char *peer, const char *reason);
extern void call_connection_closed(void *call_owner);
extern void ep_call_established(void *gateway, const char *peer, struct call *call, const char *audio_dir, const char* video_dir);
extern void ep_call_updated(void *gateway, const char *peer, const char *audio_dir, const char *video_dir);


static void call_event_handler(struct call *call, enum call_event ev,
			       const char *str, void *arg)
{
	struct ua *ua = arg;
	const char *peeruri;
	struct call *call2 = NULL;
	int err;

	peeruri = call_peeruri(call);

	switch (ev) {

	case CALL_EVENT_INCOMING:
                /* TODO intel webrtc
		if (contact_block_access(peeruri)) {

			info("ua: blocked access: \"%s\"\n", peeruri);

			ua_event(ua, UA_EVENT_CALL_CLOSED, call, str);
			mem_deref(call);
			break;
		} */

		switch (ua->acc->answermode) {

		case ANSWERMODE_EARLY:
			(void)call_progress(call);
			break;

		case ANSWERMODE_AUTO:
			(void)call_answer(call, 200);
			break;

		case ANSWERMODE_MANUAL:
		default:
                        ua_printf(ua, "Call incomming: %s owner ep %p\n", peeruri, ua->owner->ep);
			ua_event(ua, UA_EVENT_CALL_INCOMING, call, peeruri);
			ep_incoming_call(ua->owner->ep,
				             call_has_audio(call) ? 1 : 0,
				             call_has_video(call) ? 1 : 0,
				             peeruri);
			break;
		}
		break;

	case CALL_EVENT_RINGING:
		ua_event(ua, UA_EVENT_CALL_RINGING, call, peeruri);
		ep_peer_ringing(ua->owner->ep, peeruri);
		break;

	case CALL_EVENT_PROGRESS:
		ua_printf(ua, "Call in-progress: %s\n", peeruri);
		ua_event(ua, UA_EVENT_CALL_PROGRESS, call, peeruri);
		break;

	case CALL_EVENT_ESTABLISHED:
		ua_printf(ua, "Call established: %s\n", peeruri);
		ua_event(ua, UA_EVENT_CALL_ESTABLISHED, call, peeruri);
		ep_call_established(ua->owner->ep, peeruri, call, call_audio_dir(call), call_video_dir(call));
		break;

	case CALL_EVENT_CLOSED:
		ua_event(ua, UA_EVENT_CALL_CLOSED, call, str);
		if (call_get_owner(call)) {
			ep_call_closed(ua->owner->ep, peeruri, str);
			call_connection_closed(call_get_owner(call));
		}
		mem_deref(call);

		resume_call(ua);
		break;

	case CALL_EVENT_TRANSFER:

		/*
		 * Create a new call to transfer target.
		 *
		 * NOTE: we will automatically connect a new call to the
		 *       transfer target
		 */

		ua_printf(ua, "transferring call to %s\n", str);

		err = ua_call_alloc(&call2, ua, VIDMODE_ON, NULL, call,
				    call_localuri(call));
		if (!err) {
			struct pl pl;

			pl_set_str(&pl, str);

			err = call_connect(call2, &pl);
			if (err) {
				warning("ua: transfer: connect error: %m\n",
					err);
			}
		}

		if (err) {
			(void)call_notify_sipfrag(call, 500, "Call Error");
			mem_deref(call2);
		}
		break;

	case CALL_EVENT_TRANSFER_FAILED:
		ua_event(ua, UA_EVENT_CALL_TRANSFER_FAILED, call, str);
		break;
	case CALL_EVENT_UPDATE:
		if (call_get_owner(call)) {
			ep_call_updated(ua->owner->ep, peeruri, call_audio_dir(call), call_video_dir(call));
			//Now Sip call upate requires server stream connection reset
			call_connection_closed(call_get_owner(call));
	    }
	    break;
	}
}


static void call_dtmf_handler(struct call *call, char key, void *arg)
{
	struct ua *ua = arg;
	char key_str[2];

	if (key != '\0') {

		key_str[0] = key;
		key_str[1] = '\0';

		ua_event(ua, UA_EVENT_CALL_DTMF_START, call, key_str);
	}
	else {
		ua_event(ua, UA_EVENT_CALL_DTMF_END, call, NULL);
	}
}


int ua_call_alloc(struct call **callp, struct ua *ua,
			 enum vidmode vidmode, const struct sip_msg *msg,
			 struct call *xcall, const char *local_uri)
{
	struct call_prm cprm;
	int err;

	if (*callp) {
		warning("ua: call_alloc: call is already allocated\n");
		return EALREADY;
	}

	/* 1. if AF_MEDIA is set, we prefer it
	 * 2. otherwise fall back to SIP AF
	 */

	cprm.vidmode = vidmode;
	cprm.af      = ua->af;

	err = call_alloc(callp, conf_config(), &ua->calls,
			 ua->acc->dispname,
			 local_uri ? local_uri : ua->acc->aor,
			 ua->acc, ua, &cprm,
			 msg, xcall, call_event_handler, ua);
	if (err)
		return err;

	call_set_handlers(*callp, NULL, call_dtmf_handler, ua);

	return 0;
}


void ua_handle_options(struct ua *ua, const struct sip_msg *msg)
{
	struct uag *uag = ua->owner;
	struct sip_contact contact;
	struct call *call = NULL;
	struct mbuf *desc = NULL;
	int err;

        if (!uag){
        	warning("handle_options: uag is NULL\n");
        	return;
        }

	err = ua_call_alloc(&call, ua, VIDMODE_ON, NULL, NULL, NULL);
	if (err) {
		(void)sip_treply(NULL, uag->sip, msg, 500, "Call Error");
		return;
	}

	err = call_sdp_get(call, &desc, true);
	if (err)
		goto out;

	sip_contact_set(&contact, ua_cuser(ua), &msg->dst, msg->tp);

	err = sip_treplyf(NULL, NULL, uag->sip,
			  msg, true, 200, "OK",
			  "Allow: %s\r\n"
			  "%H"
			  "%H"
			  "Content-Type: application/sdp\r\n"
			  "Content-Length: %zu\r\n"
			  "\r\n"
			  "%b",
			  uag_allowed_methods(),
			  ua_print_supported, ua,
			  sip_contact_print, &contact,
			  mbuf_get_left(desc),
			  mbuf_buf(desc),
			  mbuf_get_left(desc));
	if (err) {
		warning("ua: options: sip_treplyf: %m\n", err);
	}

 out:
	mem_deref(desc);
	mem_deref(call);
}


static void ua_destructor(void *arg)
{
	struct ua *ua = arg;
        // struct uag *uag = ua->owner;

	if (ua->uap) {
		*ua->uap = NULL;
		ua->uap = NULL;
	}

	list_unlink(&ua->le);

	if (!list_isempty(&ua->regl)) {
		ua_event(ua, UA_EVENT_UNREGISTERING, NULL, NULL);
		ua_unregister(ua);
	}

	list_flush(&ua->calls);
	list_flush(&ua->regl);
	mem_deref(ua->cuser);
	mem_deref(ua->acc);
        // TODO intel webrtc
        #if 0
	if (list_isempty(uag->ual)) {
		sip_close(uag->sip, false);
	}
	#endif
}


static void add_extension(struct ua *ua, const char *extension)
{
	struct pl e;

	if (ua->extensionc >= ARRAY_SIZE(ua->extensionv)) {
		warning("ua: maximum %u number of SIP extensions\n");
		return;
	}

	pl_set_str(&e, extension);

	ua->extensionv[ua->extensionc++] = e;
}


/**
 * Allocate a SIP User-Agent
 *
 * @param uap   Pointer to allocated User-Agent object
 * @param aor   SIP Address-of-Record (AOR)
 *
 * @return 0 if success, otherwise errorcode
 */
int ua_alloc(struct ua **uap, const char *aor, struct uag *uag)
{
	struct ua *ua;
	char *buf = NULL;
	int err;

	if (!aor)
		return EINVAL;

	ua = mem_zalloc(sizeof(*ua), ua_destructor);
	if (!ua)
		return ENOMEM;

	list_init(&ua->calls);
	ua->owner = uag;

#if HAVE_INET6
	ua->af   = uag->prefer_ipv6 ? AF_INET6 : AF_INET;
#else
	ua->af   = AF_INET;
#endif

	/* Decode SIP address */
	/*
	if (uag.eprm) {
		err = re_sdprintf(&buf, "%s;%s", aor, uag.eprm);
		if (err)
			goto out;
		aor = buf;
	}
	*/

	err = account_alloc(&ua->acc, aor);
	if (err)
		goto out;

	/* generate a unique contact-user, this is needed to route
	   incoming requests when using multiple useragents */
	err = re_sdprintf(&ua->cuser, "%r-%p", &ua->acc->luri.user, ua);
	if (err)
		goto out;

	if (ua->acc->sipnat) {
		ua_printf(ua, "Using sipnat: `%s'\n", ua->acc->sipnat);
	}

	if (ua->acc->mnat) {
		ua_printf(ua, "Using medianat `%s'\n",
			  ua->acc->mnat->id);

		if (0 == str_casecmp(ua->acc->mnat->id, "ice"))
			add_extension(ua, "ice");
	}

	if (ua->acc->menc) {
		ua_printf(ua, "Using media encryption `%s'\n",
			  ua->acc->menc->id);
	}

	/* Register clients */
	if (uag->cfg && str_isset(uag->cfg->uuid))
	        add_extension(ua, "gruu");

	if (ua->acc->regint) {
		err = reg_add(&ua->regl, ua, 0);
	}
	if (err)
		goto out;

	// list_append(&uag->ual, &ua->le, ua);

	if (ua->acc->regint) {
		err = ua_register(ua);
	}

	if (!uag_current(uag))
		uag_current_set(uag, ua);

 out:
	mem_deref(buf);
	if (err)
		mem_deref(ua);
	else if (uap) {
		*uap = ua;

		ua->uap = uap;
	}

	return err;
}


static int uri_complete(struct ua *ua, struct mbuf *buf, const char *uri)
{
	size_t len;
	int err = 0;

	/* Skip initial whitespace */
	while (isspace(*uri))
		++uri;

	len = str_len(uri);

	/* Append sip: scheme if missing */
	if (0 != re_regex(uri, len, "sip:"))
		err |= mbuf_printf(buf, "sip:");

	err |= mbuf_write_str(buf, uri);

	/* Append domain if missing */
	if (0 != re_regex(uri, len, "[^@]+@[^]+", NULL, NULL)) {
#if HAVE_INET6
		if (AF_INET6 == ua->acc->luri.af)
			err |= mbuf_printf(buf, "@[%r]",
					   &ua->acc->luri.host);
		else
#endif
			err |= mbuf_printf(buf, "@%r",
					   &ua->acc->luri.host);

		/* Also append port if specified and not 5060 */
		switch (ua->acc->luri.port) {

		case 0:
		case SIP_PORT:
			break;

		default:
			err |= mbuf_printf(buf, ":%u", ua->acc->luri.port);
			break;
		}
	}

	return err;
}


/**
 * Connect an outgoing call to a given SIP uri
 *
 * @param ua        User-Agent
 * @param callp     Optional pointer to allocated call object
 * @param from_uri  Optional From uri, or NULL for default AOR
 * @param uri       SIP uri to connect to
 * @param params    Optional URI parameters
 * @param vmode     Video mode
 *
 * @return 0 if success, otherwise errorcode
 */
int ua_connect(struct ua *ua, struct call **callp,
	       const char *from_uri, const char *uri,
	       const char *params, enum vidmode vmode)
{
	struct call *call = NULL;
	struct mbuf *dialbuf;
	struct pl pl;
	int err = 0;

	if (!ua || !str_isset(uri))
		return EINVAL;

	dialbuf = mbuf_alloc(64);
	if (!dialbuf)
		return ENOMEM;

	if (params)
		err |= mbuf_printf(dialbuf, "<");

	err |= uri_complete(ua, dialbuf, uri);

	if (params) {
		err |= mbuf_printf(dialbuf, ";%s", params);
	}

	/* Append any optional URI parameters */
	err |= mbuf_write_pl(dialbuf, &ua->acc->luri.params);

	if (params)
		err |= mbuf_printf(dialbuf, ">");

	if (err)
		goto out;

	err = ua_call_alloc(&call, ua, vmode, NULL, NULL, from_uri);
	if (err)
		goto out;

	pl.p = (char *)dialbuf->buf;
	pl.l = dialbuf->end;

	err = call_connect(call, &pl);

	if (err)
		mem_deref(call);
	else if (callp)
		*callp = call;

 out:
	mem_deref(dialbuf);

	return err;
}


/**
 * Hangup the current call
 *
 * @param ua     User-Agent
 * @param call   Call to hangup, or NULL for current call
 * @param scode  Optional status code
 * @param reason Optional reason
 */
void ua_hangup(struct ua *ua, struct call *call,
	       uint16_t scode, const char *reason)
{
	if (!ua)
		return;

	if (!call) {
		call = ua_call(ua);
		if (!call)
			return;
	}

	(void)call_hangup(call, scode, reason);

	ua_event(ua, UA_EVENT_CALL_CLOSED, call, reason);

	mem_deref(call);

	resume_call(ua);
}


/**
 * Answer an incoming call
 *
 * @param ua   User-Agent
 * @param call Call to answer, or NULL for current call
 *
 * @return 0 if success, otherwise errorcode
 */
int ua_answer(struct ua *ua, struct call *call)
{
	if (!ua)
		return EINVAL;

	if (!call) {
		call = ua_call(ua);
		if (!call)
			return ENOENT;
	}

	return call_answer(call, 200);
}


/**
 * Put the current call on hold and answer the incoming call
 *
 * @param ua   User-Agent
 * @param call Call to answer, or NULL for current call
 *
 * @return 0 if success, otherwise errorcode
 */
int ua_hold_answer(struct ua *ua, struct call *call)
{
	struct call *pcall;
	int err;

	if (!ua)
		return EINVAL;

	if (!call) {
		call = ua_call(ua);
		if (!call)
			return ENOENT;
	}

	/* put previous call on-hold */
	pcall = ua_prev_call(ua);
	if (pcall) {
		ua_printf(ua, "putting call with '%s' on hold\n",
		     call_peeruri(pcall));

		err = call_hold(pcall, true);
		if (err)
			return err;
	}

	return ua_answer(ua, call);
}


int ua_print_status(struct re_printf *pf, const struct ua *ua)
{
	struct le *le;
	int err;

	if (!ua)
		return 0;

	err = re_hprintf(pf, "%-42s", ua->acc->aor);

	for (le = ua->regl.head; le; le = le->next)
		err |= reg_status(pf, le->data);

	err |= re_hprintf(pf, "\n");

	return err;
}


/**
 * Send SIP OPTIONS message to a peer
 *
 * @param ua      User-Agent object
 * @param uri     Peer SIP Address
 * @param resph   Response handler
 * @param arg     Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int ua_options_send(struct ua *ua, const char *uri,
		    options_resp_h *resph, void *arg)
{
	struct mbuf *dialbuf;
	int err = 0;

	(void)arg;

	if (!ua || !str_isset(uri))
		return EINVAL;

	dialbuf = mbuf_alloc(64);
	if (!dialbuf)
		return ENOMEM;

	err = uri_complete(ua, dialbuf, uri);
	if (err)
		goto out;

	dialbuf->buf[dialbuf->end] = '\0';

	err = sip_req_send(ua, "OPTIONS", (char *)dialbuf->buf, resph, NULL,
			   "Accept: application/sdp\r\n"
			   "Content-Length: 0\r\n"
			   "\r\n");
	if (err) {
		warning("ua: send options: (%m)\n", err);
	}

 out:
	mem_deref(dialbuf);

	return err;
}


/**
 * Get the AOR of a User-Agent
 *
 * @param ua User-Agent object
 *
 * @return AOR
 */
const char *ua_aor(const struct ua *ua)
{
	return ua ? ua->acc->aor : NULL;
}


/**
 * Get presence status of a User-Agent
 *
 * @param ua User-Agent object
 *
 * @return presence status
 */
enum presence_status ua_presence_status(const struct ua *ua)
{
	return ua ? ua->my_status : PRESENCE_UNKNOWN;
}


/**
 * Set presence status of a User-Agent
 *
 * @param ua     User-Agent object
 * @param status Presence status
 */
void ua_presence_status_set(struct ua *ua, const enum presence_status status)
{
	if (!ua)
		return;

	ua->my_status = status;
}


/**
 * Get the outbound SIP proxy of a User-Agent
 *
 * @param ua User-Agent object
 *
 * @return Outbound SIP proxy uri
 */
const char *ua_outbound(const struct ua *ua)
{
	/* NOTE: we pick the first outbound server, should be rotated? */
	return ua ? ua->acc->outbound[0] : NULL;
}


/**
 * Get the current call object of a User-Agent
 *
 * @param ua User-Agent object
 *
 * @return Current call, NULL if no active calls
 *
 *
 * Current call strategy:
 *
 * We can only have 1 current call. The current call is the one that was
 * added last (end of the list), which is not on-hold
 */
struct call *ua_call(const struct ua *ua)
{
	struct le *le;

	if (!ua)
		return NULL;

	for (le = ua->calls.tail; le; le = le->prev) {

		struct call *call = le->data;

		/* todo: check if call is on-hold */

		return call;
	}

	return NULL;
}

struct call *ua_prev_call(const struct ua *ua)
{
	struct le *le;
	int prev = 0;

	if (!ua)
		return NULL;

	for (le = ua->calls.tail; le; le = le->prev) {
		if ( prev == 1) {
			struct call *call = le->data;
			return call;
		}
		if ( prev == 0)
			prev = 1;
	}

	return NULL;
}


int ua_debug(struct re_printf *pf, const struct ua *ua)
{
	struct le *le;
	int err;

	if (!ua)
		return 0;

	err  = re_hprintf(pf, "--- %s ---\n", ua->acc->aor);
	err |= re_hprintf(pf, " nrefs:     %u\n", mem_nrefs(ua));
	err |= re_hprintf(pf, " cuser:     %s\n", ua->cuser);
	err |= re_hprintf(pf, " af:        %s\n", net_af2name(ua->af));
	err |= re_hprintf(pf, " %H", ua_print_supported, ua);

	err |= account_debug(pf, ua->acc);

	for (le = ua->regl.head; le; le = le->next)
		err |= reg_debug(pf, le->data);

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
#if 0
int ua_print_sip_status(struct re_printf *pf, void *unused)
{
	(void)unused;
	return sip_debug(pf, uag.sip);
}
#endif


/**
 * Print all calls for a given User-Agent
 */
int ua_print_calls(struct re_printf *pf, const struct ua *ua)
{
	struct le *le;
	int err = 0;
	if (!ua) {
		err |= re_hprintf(pf, "\n--- No active calls ---\n");
		return err;
	}

	err |= re_hprintf(pf, "\n--- List of active calls (%u): ---\n",
			  list_count(&ua->calls));

	for (le = ua->calls.head; le; le = le->next) {

		const struct call *call = le->data;

		err |= re_hprintf(pf, "  %H\n", call_info, call);
	}

	err |= re_hprintf(pf, "\n");

	return err;
}


/**
 * Get the current SIP socket file descriptor for a User-Agent
 *
 * @param ua User-Agent
 *
 * @return File descriptor, or -1 if not available
 */
int ua_sipfd(const struct ua *ua)
{
	struct le *le;

	if (!ua)
		return -1;

	for (le = ua->regl.head; le; le = le->next) {

		struct reg *reg = le->data;
		int fd;

		fd = reg_sipfd(reg);
		if (fd != -1)
			return fd;
	}

	return -1;
}


int ua_print_supported(struct re_printf *pf, const struct ua *ua)
{
	size_t i;
	int err;

	err = re_hprintf(pf, "Supported:");

	for (i=0; i<ua->extensionc; i++) {
		err |= re_hprintf(pf, "%s%r",
				  i==0 ? " " : ",", &ua->extensionv[i]);
	}

	err |= re_hprintf(pf, "\r\n");

	return err;
}


struct account *ua_prm(const struct ua *ua)
{
	return ua ? ua->acc : NULL;
}


struct list *ua_calls(const struct ua *ua)
{
	return ua ? (struct list *)&ua->calls : NULL;
}

const char *ua_local_cuser(const struct ua *ua)
{
        return ua ? ua->cuser : NULL;
}
