/**
 * @file src/call.c  Call Control
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <re/re.h>
#include <baresip.h>
#include "core.h"

#define CHECK_AV_INTERVAL 6000

#define FOREACH_STREAM						\
	for (le = call->streaml.head; le; le = le->next)

/** Call constants */
enum {
	PTIME           = 20,    /**< Packet time for audio               */
	LOCAL_TIMEOUT   = 120,   /**< Incoming call timeout in [seconds]  */
};


/** Call States */
enum state {
	STATE_IDLE = 0,
	STATE_INCOMING,
	STATE_OUTGOING,
	STATE_RINGING,
	STATE_EARLY,
	STATE_ESTABLISHED,
	STATE_TERMINATED
};

/** SIP Call Control object */
struct call {
	struct le le;             /**< Linked list element                  */
	struct ua *ua;            /**< SIP User-agent                       */
	struct account *acc;      /**< Account (ref.)                       */
	struct sipsess *sess;     /**< SIP Session                          */
	struct sdp_session *sdp;  /**< SDP Session                          */
	struct sipsub *sub;       /**< Call transfer REFER subscription     */
	struct sipnot *not;       /**< REFER/NOTIFY client                  */
	struct list streaml;      /**< List of mediastreams (struct stream) */
	void *owner;              /**< The owner SipCallConnection          */
	struct audio *audio;      /**< Audio stream                         */
#ifdef USE_VIDEO
	struct video *video;      /**< Video stream                         */
	struct bfcp *bfcp;        /**< BFCP Client                          */
#endif
	enum state state;         /**< Call state                           */
	char *local_uri;          /**< Local SIP uri                        */
	char *local_name;         /**< Local display name                   */
	char *peer_uri;           /**< Peer SIP Address                     */
	char *peer_name;          /**< Peer display name                    */
	struct tmr tmr_inv;       /**< Timer for incoming calls             */
	struct tmr tmr_dtmf;      /**< Timer for incoming DTMF events       */
    struct tmr tmr_check_av_rx;/**< Timer for checking connection timeout*/
	time_t time_start;        /**< Time when call started               */
	time_t time_conn;         /**< Time when call initiated             */
	time_t time_stop;         /**< Time when call stopped               */
	bool outgoing;
	bool got_offer;           /**< Got SDP Offer from Peer              */
	bool on_hold;             /**< True if call is on hold              */
	struct mnat_sess *mnats;  /**< Media NAT session                    */
	bool mnat_wait;           /**< Waiting for MNAT to establish        */
	struct menc_sess *mencs;  /**< Media encryption session state       */
	int af;                   /**< Preferred Address Family             */
	uint16_t scode;           /**< Termination status code              */
	call_event_h *eh;         /**< Event handler                        */
	call_dtmf_h *dtmfh;       /**< DTMF handler                         */
	void *arg;                /**< Handler argument                     */

	struct config_avt config_avt;
};


static int send_invite(struct call *call);


static const char *state_name(enum state st)
{
	switch (st) {

	case STATE_IDLE:        return "IDLE";
	case STATE_INCOMING:    return "INCOMING";
	case STATE_OUTGOING:    return "OUTGOING";
	case STATE_RINGING:     return "RINGING";
	case STATE_EARLY:       return "EARLY";
	case STATE_ESTABLISHED: return "ESTABLISHED";
	case STATE_TERMINATED:  return "TERMINATED";
	default:                return "???";
	}
}


static void set_state(struct call *call, enum state st)
{
	call->state = st;
}

extern void ep_call_closed(void *gateway, const char *peer, const char *reason);
extern void ep_call_loss(void *gateway, const char *peer, const char *reason, void *call);
extern int get_video_counter(const struct video *v);
extern int get_audio_counter(const struct audio *a);
extern void reset_video_counter(struct video *v);
extern void reset_audio_counter(struct audio *a);
static void check_audio_video_rx(void *arg){
    struct call *call = arg;

    if(!call)
        return;

    int video_frames = get_video_counter(call->video);
    int audio_frames = get_audio_counter(call->audio);

    bool video_stopped = video_frames == 0;
    bool audio_stopped = audio_frames == 0;

    if(video_stopped && audio_stopped){
        ep_call_loss((void *)call->ua->owner->ep, call_peeruri(call), "connection loss", call);
    }else{
        reset_video_counter(call->video);
        reset_audio_counter(call->audio);

        tmr_start(&call->tmr_check_av_rx, CHECK_AV_INTERVAL, check_audio_video_rx, call);
    }
}

void start_tmr_check_av_if_needed(struct call *call){
    if(!call)
        return;

    struct sdp_media *video_media = stream_sdpmedia(video_strm(call->video));
    struct sdp_media *audio_media = stream_sdpmedia(audio_strm(call->audio));
    enum sdp_dir video_dir = sdp_media_has_media(video_media) ? sdp_media_dir(video_media) : 0;
    enum sdp_dir audio_dir = sdp_media_has_media(audio_media) ? sdp_media_dir(audio_media) : 0;

    if(video_dir == SDP_SENDONLY && audio_dir == SDP_SENDONLY){
        tmr_cancel(&call->tmr_check_av_rx);
    }else if(!call->tmr_check_av_rx.th){
        tmr_start(&call->tmr_check_av_rx, CHECK_AV_INTERVAL, check_audio_video_rx, call);
    }
}

static void call_stream_start(struct call *call, bool active)
{
	const struct sdp_format *sc;
	int err;

	/* Audio Stream */
	sc = sdp_media_rformat(stream_sdpmedia(audio_strm(call->audio)), NULL);
	if (sc) {
		struct aucodec *ac = sc->data;

		if (ac) {
			err  = audio_encoder_set(call->audio, sc->data,
						 sc->pt, sc->params);
			err |= audio_decoder_set(call->audio, sc->data,
						 sc->pt, sc->params);
			if (!err) {
				err = audio_start(call->audio);
			}
			if (err) {
				warning("call: audio stream error: %m\n", err);
			}
		}
		else {
			info("call: no common audio-codecs..\n");
		}
	}
	else {
		info("call: audio stream is disabled..\n");
	}

#ifdef USE_VIDEO
	/* Video Stream */
	sc = sdp_media_rformat(stream_sdpmedia(video_strm(call->video)), NULL);
	if (sc) {
		err  = video_encoder_set(call->video, sc->data, sc->pt,
					 sc->params);
		err |= video_decoder_set(call->video, sc->data, sc->pt,
					 sc->rparams);
		if (!err) {
			err = video_start(call->video, call->peer_uri);
		}
		if (err) {
			warning("call: video stream error: %m\n", err);
		}
	}
	else if (call->video) {
		info("call: video stream is disabled..\n");
	}

	if (call->bfcp) {
		err = bfcp_start(call->bfcp);
		if (err) {
			warning("call: could not start BFCP: %m\n", err);
		}
	}
#endif

	if (active) {
		struct le *le;

		tmr_cancel(&call->tmr_inv);
		call->time_start = time(NULL);

		FOREACH_STREAM {
			stream_reset(le->data);
		}
	}
}

static void call_stream_stop(struct call *call)
{
	if (!call)
		return;

	call->time_stop = time(NULL);
	tmr_cancel(&call->tmr_inv);
}



static void call_event_handler(struct call *call, enum call_event ev,
			       const char *fmt, ...)
{
	call_event_h *eh = call->eh;
	void *eh_arg = call->arg;
	char buf[256];
	va_list ap;

	if (!eh)
		return;

	va_start(ap, fmt);
	(void)re_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	eh(call, ev, buf, eh_arg);
}


static void invite_timeout(void *arg)
{
	struct call *call = arg;

	info("%s: Local timeout after %u seconds\n",
	     call->peer_uri, LOCAL_TIMEOUT);

	call_event_handler(call, CALL_EVENT_CLOSED, "Local timeout");
}


/** Called when all media streams are established */
static void mnat_handler(int err, uint16_t scode, const char *reason,
			 void *arg)
{
	struct call *call = arg;

	if (err) {
		warning("call: medianat '%s' failed: %m\n",
			call->acc->mnatid, err);
		call_event_handler(call, CALL_EVENT_CLOSED, "%m", err);
		return;
	}
	else if (scode) {
		warning("call: medianat failed: %u %s\n", scode, reason);
		call_event_handler(call, CALL_EVENT_CLOSED, "%u %s",
				   scode, reason);
		return;
	}

	/* Re-INVITE */
	if (!call->mnat_wait) {
		info("call: medianat established -- sending Re-INVITE\n");
		(void)call_modify(call);
		return;
	}

	call->mnat_wait = false;

	switch (call->state) {

	case STATE_OUTGOING:
		(void)send_invite(call);
		break;

	case STATE_INCOMING:
		call_event_handler(call, CALL_EVENT_INCOMING, call->peer_uri);
		break;

	default:
		break;
	}
}


static int update_media(struct call *call)
{
	const struct sdp_format *sc;
	struct le *le;
	int err = 0;

	/* media attributes */
	audio_sdp_attr_decode(call->audio);

#ifdef USE_VIDEO
	if (call->video)
		video_sdp_attr_decode(call->video);
#endif

	/* Update each stream */
	FOREACH_STREAM {
		stream_update(le->data);
	}

	if (call->acc->mnat && call->acc->mnat->updateh && call->mnats)
		err = call->acc->mnat->updateh(call->mnats);

	sc = sdp_media_rformat(stream_sdpmedia(audio_strm(call->audio)), NULL);
	if (sc) {
		struct aucodec *ac = sc->data;
		if (ac) {
			err  = audio_decoder_set(call->audio, sc->data,
						 sc->pt, sc->params);
			err |= audio_encoder_set(call->audio, sc->data,
						 sc->pt, sc->params);
		}
		else {
			info("no common audio-codecs..\n");
		}
	}
	else {
		info("audio stream is disabled..\n");
	}

#ifdef USE_VIDEO
	sc = sdp_media_rformat(stream_sdpmedia(video_strm(call->video)), NULL);
	if (sc) {
		err = video_encoder_set(call->video, sc->data,
					sc->pt, sc->params);
		if (err) {
			warning("call: video stream error: %m\n", err);
		}
	}
	else if (call->video) {
		info("video stream is disabled..\n");
	}
#endif

	return err;
}


static void print_summary(const struct call *call)
{
	uint32_t dur = call_duration(call);
	if (!dur)
		return;

	info("%s: Call with %s terminated (duration: %H)\n",
	     call->local_uri, call->peer_uri, fmt_human_time, &dur);
}


static void call_destructor(void *arg)
{
	struct call *call = arg;

	if (call->state != STATE_IDLE)
		print_summary(call);

	call_stream_stop(call);
	list_unlink(&call->le);
	tmr_cancel(&call->tmr_dtmf);
	tmr_cancel(&call->tmr_check_av_rx);

	mem_deref(call->sess);
	mem_deref(call->local_uri);
	mem_deref(call->local_name);
	mem_deref(call->peer_uri);
	mem_deref(call->peer_name);
	mem_deref(call->audio);
#ifdef USE_VIDEO
	mem_deref(call->video);
	mem_deref(call->bfcp);
#endif
	mem_deref(call->sdp);
	mem_deref(call->mnats);
	mem_deref(call->mencs);
	mem_deref(call->sub);
	mem_deref(call->not);
	mem_deref(call->acc);
}


static void audio_event_handler(int key, bool end, void *arg)
{
	struct call *call = arg;

	info("received event: '%c' (end=%d)\n", key, end);

	if (call->dtmfh)
		call->dtmfh(call, end ? 0x00 : key, call->arg);
}


static void audio_error_handler(int err, const char *str, void *arg)
{
	struct call *call = arg;

	if (err) {
		warning("call: audio device error: %m (%s)\n", err, str);
	}

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, str);
}


#ifdef USE_VIDEO
static void video_error_handler(int err, const char *str, void *arg)
{
	struct call *call = arg;

	warning("call: video device error: %m (%s)\n", err, str);

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, str);
}
#endif


static void menc_error_handler(int err, void *arg)
{
	struct call *call = arg;

	warning("call: mediaenc '%s' error: %m\n", call->acc->mencid, err);

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, "mediaenc failed");
}

/**
 * Allocate a new Call state object
 *
 * @param callp       Pointer to allocated Call state object
 * @param cfg         Global configuration
 * @param lst         List of call objects
 * @param local_name  Local display name (optional)
 * @param local_uri   Local SIP uri
 * @param acc         Account parameters
 * @param ua          User-Agent
 * @param prm         Call parameters
 * @param msg         SIP message for incoming calls
 * @param xcall       Optional call to inherit properties from
 * @param eh          Call event handler
 * @param arg         Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int call_alloc(struct call **callp, const struct config *cfg, struct list *lst,
	       const char *local_name, const char *local_uri,
	       struct account *acc, struct ua *ua, const struct call_prm *prm,
	       const struct sip_msg *msg, struct call *xcall,
	       call_event_h *eh, void *arg)
{
	struct call *call;
	enum vidmode vidmode = prm ? prm->vidmode : VIDMODE_OFF;
	bool use_video = true, got_offer = false;
	int label = 0;
	int err = 0;

	if (!cfg || !local_uri || !acc || !ua)
		return EINVAL;

	call = mem_zalloc(sizeof(*call), call_destructor);
	if (!call)
		return ENOMEM;


	call->config_avt = cfg->avt;

	tmr_init(&call->tmr_inv);

	call->acc    = mem_ref(acc);
	call->ua     = ua;
	call->state  = STATE_IDLE;
	call->eh     = eh;
	call->arg    = arg;
	call->af     = prm ? prm->af : AF_INET;

	err = str_dup(&call->local_uri, local_uri);
	if (local_name)
		err |= str_dup(&call->local_name, local_name);
	if (err)
		goto out;

	/* Init SDP info */
	err = sdp_session_alloc(&call->sdp, net_laddr_af(call->af));
	if (err)
		goto out;

	err = sdp_session_set_lattr(call->sdp, true,
				    "tool", "intel-sipua " SIPUA_VERSION);
	if (err)
		goto out;

	/* Check for incoming SDP Offer */
	if (msg && mbuf_get_left(msg->mb))
		got_offer = true;

	/* Initialise media NAT handling */
	if (acc->mnat) {
		err = acc->mnat->sessh(&call->mnats, net_dnsc(), call->af,
				       acc->stun_host, acc->stun_port,
				       acc->stun_user, acc->stun_pass,
				       call->sdp, !got_offer,
				       mnat_handler, call);
		if (err) {
			warning("call: medianat session: %m\n", err);
			goto out;
		}
	}
	call->mnat_wait = true;

	/* Media encryption */
	if (acc->menc) {
		if (acc->menc->sessh) {
			err = acc->menc->sessh(&call->mencs, call->sdp,
						!got_offer,
						menc_error_handler, call);
			if (err) {
				warning("call: mediaenc session: %m\n", err);
				goto out;
			}
		}
	}

	/* Audio stream */
	err = audio_alloc(&call->audio, cfg, call,
			  call->sdp, ++label,
			  acc->mnat, call->mnats, acc->menc, call->mencs,
			  acc->ptime, account_aucodecl(call->acc),
			  audio_event_handler, audio_error_handler, call);
	if (err)
		goto out;

#ifdef USE_VIDEO
	/* We require at least one video codec, and at least one
	   video source or video display */
	use_video = (vidmode != VIDMODE_OFF)
		&& (list_head(account_vidcodecl(call->acc)) != NULL);

	/* Video stream */
	if (use_video) {
 		err = video_alloc(&call->video, cfg,
				  call, call->sdp, ++label,
				  acc->mnat, call->mnats,
				  acc->menc, call->mencs,
				  "main",
				  account_vidcodecl(call->acc),
				  video_error_handler, call);
		if (err)
			goto out;
 	}

	if (str_isset(cfg->bfcp.proto)) {

		err = bfcp_alloc(&call->bfcp, call->sdp,
				 cfg->bfcp.proto, !got_offer,
				 acc->mnat, call->mnats, call->ua->owner);
		if (err)
			goto out;
	}
#else
	(void)use_video;
	(void)vidmode;
#endif

	/* inherit certain properties from original call */
	if (xcall) {
		call->not = mem_ref(xcall->not);
	}

	list_append(lst, &call->le, call);

 out:
	if (err)
		mem_deref(call);
	else if (callp)
		*callp = call;

	return err;
}


int call_connect(struct call *call, const struct pl *paddr)
{
	struct sip_addr addr;
	int err;

	if (!call || !paddr)
		return EINVAL;

	info("call: connecting to '%r'..\n", paddr);

	call->outgoing = true;

	/* if the peer-address is a full SIP address then we need
	 * to parse it and extract the SIP uri part.
	 */
	if (0 == sip_addr_decode(&addr, paddr) && addr.dname.p) {
		err = pl_strdup(&call->peer_uri, &addr.auri);
	}
	else {
		err = pl_strdup(&call->peer_uri, paddr);
	}
	if (err)
		return err;

	set_state(call, STATE_OUTGOING);

	/* If we are using asyncronous medianat like STUN/TURN, then
	 * wait until completed before sending the INVITE */
	if (!call->acc->mnat)
		err = send_invite(call);

	return err;
}


/**
 * Update the current call by sending Re-INVITE or UPDATE
 *
 * @param call Call object
 *
 * @return 0 if success, otherwise errorcode
 */
int call_modify(struct call *call)
{
	struct mbuf *desc;
	int err;

	if (!call)
		return EINVAL;

	err = call_sdp_get(call, &desc, true);
	if (!err)
		err = sipsess_modify(call->sess, desc);

	mem_deref(desc);

	return err;
}


int call_hangup(struct call *call, uint16_t scode, const char *reason)
{
	int err = 0;

	if (!call)
		return EINVAL;

	switch (call->state) {

	case STATE_INCOMING:
		if (scode < 400) {
			scode = 486;
			reason = "Rejected";
		}
		info("call: rejecting incoming call from %s (%u %s)\n",
		     call->peer_uri, scode, reason);
		(void)sipsess_reject(call->sess, scode, reason, NULL);
		break;

	default:
		info("call: terminate call with %s\n", call->peer_uri);
		call->sess = mem_deref(call->sess);
		break;
	}

	set_state(call, STATE_TERMINATED);

	call_stream_stop(call);

	return err;
}


int call_progress(struct call *call)
{
	struct mbuf *desc;
	int err;

	if (!call)
		return EINVAL;

	tmr_cancel(&call->tmr_inv);

	err = call_sdp_get(call, &desc, false);
	if (err)
		return err;

	err = sipsess_progress(call->sess, 183, "Session Progress",
			       desc, "Allow: %s\r\n", uag_allowed_methods());

	if (!err)
		call_stream_start(call, false);

	mem_deref(desc);

	return 0;
}


int call_answer(struct call *call, uint16_t scode)
{
	struct mbuf *desc;
	int err;

	if (!call || !call->sess)
		return EINVAL;

	if (STATE_INCOMING != call->state) {
		info("call: answer: call is not in incoming state (%s)\n",
		     state_name(call->state));
		return 0;
	}

	info("answering call from %s with %u\n", call->peer_uri, scode);

	if (call->got_offer) {

		err = update_media(call);
		if (err)
			return err;
	}

	err = sdp_encode(&desc, call->sdp, !call->got_offer);
	if (err)
		return err;

	err = sipsess_answer(call->sess, scode, "Answering", desc,
			     "Allow: %s\r\n", uag_allowed_methods());

	mem_deref(desc);

	return err;
}

/**
 * Check if the current call has an active audio stream
 *
 * @param call  Call object
 *
 * @return True if active stream, otherwise false
 */
bool call_has_audio(const struct call *call)
{
	if (!call)
		return false;

	return sdp_media_has_media(stream_sdpmedia(audio_strm(call->audio)));
}


/**
 * Check if the current call has an active video stream
 *
 * @param call  Call object
 *
 * @return True if active stream, otherwise false
 */
bool call_has_video(const struct call *call)
{
	if (!call)
		return false;

#ifdef USE_VIDEO
	return sdp_media_has_media(stream_sdpmedia(video_strm(call->video)));
#else
	return false;
#endif
}

/**
 *	@return Remote audio direction in this call
 *		NULL if audio is disabled,
 *		"recvonly" for SDP_RECVONLY,
 *		"sendonly" for SDP_SENDONLY,
 *		"sendrecv" for SDP_SENDRECV
*/
const char* call_audio_dir(const struct call *call) {
    struct sdp_media *media = 0;
    if (!call)
        return NULL;
    media = stream_sdpmedia(audio_strm(call->audio));
    if (sdp_media_has_media(media)) {
        enum sdp_dir dir = sdp_media_dir(media);
        if (dir == SDP_SENDONLY || dir == SDP_RECVONLY || dir == SDP_SENDRECV) {
            return sdp_dir_name(dir);
        }
    }
    return NULL;
}

/**
 *	@return Remote audio direction in this call
 *		NULL if video is disabled,
 *		"recvonly" for SDP_RECVONLY,
 *		"sendonly" for SDP_SENDONLY,
 *		"sendrecv" for SDP_SENDRECV
*/
const char* call_video_dir(const struct call *call) {
    struct sdp_media *media = 0;
    if (!call)
        return NULL;
#ifdef USE_VIDEO
    media = stream_sdpmedia(video_strm(call->video));
    if (sdp_media_has_media(media)) {
        enum sdp_dir dir= sdp_media_dir(media);
        if (dir == SDP_SENDONLY || dir == SDP_RECVONLY || dir == SDP_SENDRECV) {
            return sdp_dir_name(dir);
        }
    }
#endif
    return NULL;
}

/**
 * Put the current call on hold/resume
 *
 * @param call  Call object
 * @param hold  True to hold, false to resume
 *
 * @return 0 if success, otherwise errorcode
 */
int call_hold(struct call *call, bool hold)
{
	struct le *le;

	if (!call || !call->sess)
		return EINVAL;

	if (hold == call->on_hold)
		return 0;

	info("call: %s %s\n", hold ? "hold" : "resume", call->peer_uri);

	call->on_hold = hold;

	FOREACH_STREAM
		stream_hold(le->data, hold);

	return call_modify(call);
}


int call_sdp_get(const struct call *call, struct mbuf **descp, bool offer)
{
	return sdp_encode(descp, call->sdp, offer);
}


const char *call_peeruri(const struct call *call)
{
	return call ? call->peer_uri : NULL;
}


const char *call_localuri(const struct call *call)
{
	return call ? call->local_uri : NULL;
}


/**
 * Get the name of the peer
 *
 * @param call  Call object
 *
 * @return Peer name
 */
const char *call_peername(const struct call *call)
{
	return call ? call->peer_name : NULL;
}


int call_debug(struct re_printf *pf, const struct call *call)
{
	int err;

	if (!call)
		return 0;

	err = re_hprintf(pf, "===== Call debug (%s) =====\n",
			 state_name(call->state));

	/* SIP Session debug */
	err |= re_hprintf(pf,
			  " local_uri: %s <%s>\n"
			  " peer_uri:  %s <%s>\n"
			  " af=%s\n",
			  call->local_name, call->local_uri,
			  call->peer_name, call->peer_uri,
			  net_af2name(call->af));
	err |= re_hprintf(pf, " direction: %s\n",
			  call->outgoing ? "Outgoing" : "Incoming");

	/* SDP debug */
	err |= sdp_session_debug(pf, call->sdp);

	return err;
}


static int print_duration(struct re_printf *pf, const struct call *call)
{
	const uint32_t dur = call_duration(call);
	const uint32_t sec = dur%60%60;
	const uint32_t min = dur/60%60;
	const uint32_t hrs = dur/60/60;

	return re_hprintf(pf, "%u:%02u:%02u", hrs, min, sec);
}


int call_status(struct re_printf *pf, const struct call *call)
{
	struct le *le;
	int err;

	if (!call)
		return EINVAL;

	switch (call->state) {

	case STATE_EARLY:
	case STATE_ESTABLISHED:
		break;
	default:
		return 0;
	}

	err = re_hprintf(pf, "\r[%H]", print_duration, call);

	FOREACH_STREAM
		err |= stream_print(pf, le->data);

	err |= re_hprintf(pf, " (bit/s)");

#ifdef USE_VIDEO
	if (call->video)
		err |= video_print(pf, call->video);
#endif

	return err;
}


int call_jbuf_stat(struct re_printf *pf, const struct call *call)
{
	struct le *le;
	int err = 0;

	if (!call)
		return EINVAL;

	FOREACH_STREAM
		err |= stream_jbuf_stat(pf, le->data);

	return err;
}


int call_info(struct re_printf *pf, const struct call *call)
{
	if (!call)
		return 0;

	return re_hprintf(pf, "%H  %9s  %s  %s", print_duration, call,
			  state_name(call->state),
			  call->on_hold ? "(on hold)" : "         ",
			  call->peer_uri);
}


/**
 * Send a DTMF digit to the peer
 *
 * @param call  Call object
 * @param key   DTMF digit to send (0x00 for key release)
 *
 * @return 0 if success, otherwise errorcode
 */
int call_send_digit(struct call *call, char key)
{
	if (!call)
		return EINVAL;

	return audio_send_digit(call->audio, key);
}


struct ua *call_get_ua(const struct call *call)
{
	return call ? call->ua : NULL;
}

void* call_get_owner(const struct call *call)
{
    return call ? call->owner : NULL;
}

void call_set_owner(struct call *call, void *owner)
{
    if (call) call->owner = owner;
}

static int auth_handler(char **username, char **password,
			const char *realm, void *arg)
{
	struct account *acc = arg;
	return account_auth(acc, username, password, realm);
}


static int sipsess_offer_handler(struct mbuf **descp,
				 const struct sip_msg *msg, void *arg)
{
	const bool got_offer = mbuf_get_left(msg->mb);
	struct call *call = arg;
	int err;


	info("call: got re-INVITE%s\n", got_offer ? " (SDP Offer)" : "");

	if (got_offer) {

		/* Decode SDP Offer */
		err = sdp_decode(call->sdp, msg->mb, true);
		if (err)
			return err;

		err = update_media(call);
		if (err)
			return err;
		call_event_handler(call, CALL_EVENT_UPDATE, "got re-INVITE");
	}

	/* Encode SDP Answer */
	err = sdp_encode(descp, call->sdp, !got_offer);
	if (!err)
		call_event_handler(call, CALL_EVENT_UPDATE, "got re-INVITE");

    start_tmr_check_av_if_needed(call);
	return err;
}


static int sipsess_answer_handler(const struct sip_msg *msg, void *arg)
{
	struct call *call = arg;
	int err;

	if (msg_ctype_cmp(&msg->ctyp, "multipart", "mixed"))
		(void)sdp_decode_multipart(&msg->ctyp.params, msg->mb);

	err = sdp_decode(call->sdp, msg->mb, false);
	if (err) {
		warning("call: could not decode SDP answer: %m\n", err);
		return err;
	}

	err = update_media(call);
	if (err)
		return err;

	return 0;
}


static void sipsess_estab_handler(const struct sip_msg *msg, void *arg)
{
	struct call *call = arg;


	(void)msg;

	if (call->state == STATE_ESTABLISHED)
		return;

	set_state(call, STATE_ESTABLISHED);

	call_stream_start(call, true);

	/* the transferor will hangup this call */
	if (call->not) {
		(void)call_notify_sipfrag(call, 200, "OK");
	}

	/* must be done last, the handler might deref this call */
	call_event_handler(call, CALL_EVENT_ESTABLISHED, call->peer_uri);

    start_tmr_check_av_if_needed(call);
}


#ifdef USE_VIDEO
static void call_handle_info_req(struct call *call, const struct sip_msg *req)
{
	struct pl body;
	bool pfu;
	int err;

	(void)call;

	pl_set_mbuf(&body, req->mb);

	err = mctrl_handle_media_control(&body, &pfu);
	if (err)
		return;

	if (pfu) {
		video_update_picture(call->video);
	}
}
#endif


static void dtmfend_handler(void *arg)
{
	struct call *call = arg;

	if (call->dtmfh)
		call->dtmfh(call, 0x00, call->arg);
}


static void sipsess_info_handler(struct sip *sip, const struct sip_msg *msg,
				 void *arg)
{
	struct call *call = arg;

	info("sipsess_info_handler: msg->ctype = %s\n", &msg->ctyp);

	if (msg_ctype_cmp(&msg->ctyp, "application", "dtmf-relay")) {

		struct pl body, sig, dur;
		int err;

		pl_set_mbuf(&body, msg->mb);

		err  = re_regex(body.p, body.l, "Signal=[0-9*#a-d]+", &sig);
		err |= re_regex(body.p, body.l, "Duration=[0-9]+", &dur);

		if (err || !pl_isset(&sig) || sig.l == 0) {
			(void)sip_reply(sip, msg, 400, "Bad Request");
		}
		else {
			char s = toupper(sig.p[0]);
			uint32_t duration = pl_u32(&dur);

			info("received DTMF: '%c' (duration=%r)\n", s, &dur);

			(void)sip_reply(sip, msg, 200, "OK");

			if (call->dtmfh) {
				tmr_start(&call->tmr_dtmf, duration,
					  dtmfend_handler, call);
				call->dtmfh(call, s, call->arg);
			}
		}
	}
#ifdef USE_VIDEO
	else if (msg_ctype_cmp(&msg->ctyp,
			       "application", "media_control+xml")) {
		call_handle_info_req(call, msg);
		(void)sip_reply(sip, msg, 200, "OK");
	}
#endif
	else {
		(void)sip_reply(sip, msg, 488, "Not Acceptable Here");
	}
}


static void sipnot_close_handler(int err, const struct sip_msg *msg,
				 void *arg)
{
	struct call *call = arg;

	if (err)
		info("call: notification closed: %m\n", err);
	else if (msg)
		info("call: notification closed: %u %r\n",
		     msg->scode, &msg->reason);

	call->not = mem_deref(call->not);
}


static void sipsess_refer_handler(struct sip *sip, const struct sip_msg *msg,
				  void *arg)
{
	struct call *call = arg;
	const struct sip_hdr *hdr;
	int err;

	/* get the transfer target */
	hdr = sip_msg_hdr(msg, SIP_HDR_REFER_TO);
	if (!hdr) {
		warning("call: bad REFER request from %r\n", &msg->from.auri);
		(void)sip_reply(sip, msg, 400, "Missing Refer-To header");
		return;
	}

	/* The REFER creates an implicit subscription.
	 * Reply 202 to the REFER request
	 */
	call->not = mem_deref(call->not);
	err = sipevent_accept(&call->not, call->ua->owner->evsock, msg,
			      sipsess_dialog(call->sess), NULL,
			      202, "Accepted", 60, 60, 60,
			      ua_cuser(call->ua), "message/sipfrag",
			      auth_handler, call->acc, true,
			      sipnot_close_handler, call,
			      "Allow: %s\r\n", uag_allowed_methods());
	if (err) {
		warning("call: refer: sipevent_accept failed: %m\n", err);
		return;
	}

	(void)call_notify_sipfrag(call, 100, "Trying");

	call_event_handler(call, CALL_EVENT_TRANSFER, "%r", &hdr->val);
}


static void sipsess_close_handler(int err, const struct sip_msg *msg,
				  void *arg)
{
	struct call *call = arg;
	char reason[128] = "";

	if (err) {
		info("%s: session closed: %m\n", call->peer_uri, err);

		if (call->not) {
			(void)call_notify_sipfrag(call, 500, "%m", err);
		}
	}
	else if (msg) {

		call->scode = msg->scode;

		(void)re_snprintf(reason, sizeof(reason), "%u %r",
				  msg->scode, &msg->reason);

		info("%s: session closed: %u %r\n",
		     call->peer_uri, msg->scode, &msg->reason);

		if (call->not) {
			(void)call_notify_sipfrag(call, msg->scode,
						  "%r", &msg->reason);
		}
	}
	else {
		info("%s: session closed\n", call->peer_uri);
	}

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, reason);
}

#if 0 
static bool have_common_audio_codecs(const struct call *call)
{
	const struct sdp_format *sc;
	struct aucodec *ac;

	sc = sdp_media_rformat(stream_sdpmedia(audio_strm(call->audio)), NULL);
	if (!sc)
		return false;

	ac = sc->data;  /* note: this will exclude telephone-event */

	return ac != NULL;
}

static sip* uag_sip( struct call *call)
{
    return call->ua->owner->sip;
}
#endif

int call_accept(struct call *call, struct sipsess_sock *sess_sock,
		const struct sip_msg *msg)
{
	bool got_offer;
	int err;

	if (!call || !msg)
		return EINVAL;

	call->outgoing = false;

	got_offer = (mbuf_get_left(msg->mb) > 0);

	err = pl_strdup(&call->peer_uri, &msg->from.auri);
	info("%s: call_accept\n", call->peer_uri);
	if (err)
		return err;

	if (pl_isset(&msg->from.dname)) {
		err = pl_strdup(&call->peer_name, &msg->from.dname);
		if (err)
			return err;
	}

	if (got_offer) {
		// struct sdp_media *m;
		// const struct sa *raddr;

		err = sdp_decode(call->sdp, msg->mb, true);
		if (err)
			return err;

		call->got_offer = true;


#if 0
// TODO intel webrtc
		/*
		 * Each media description in the SDP answer MUST
		 * use the same network type as the corresponding
		 * media description in the offer.
		 *
		 * See RFC 6157
		 */
		m = stream_sdpmedia(audio_strm(call->audio));
		raddr = sdp_media_raddr(m);

		if (sa_af(raddr) != call->af) {
			info("call: incompatible address-family"
			     " (local=%s, remote=%s)\n",
			     net_af2name(call->af),
			     net_af2name(sa_af(raddr)));

			sip_treply(NULL, uag_sip(), msg,
				   488, "Not Acceptable Here");

			call_event_handler(call, CALL_EVENT_CLOSED,
					   "Wrong address family");
			return 0;
		}

		/* Check if we have any common audio codecs, after
		 * the SDP offer has been parsed
		 */
		if (!have_common_audio_codecs(call)) {
			info("call: no common audio codecs - rejected\n");

			sip_treply(NULL, uag_sip(), msg,
				   488, "Not Acceptable Here");

			call_event_handler(call, CALL_EVENT_CLOSED,
					   "No audio codecs");

			return 0;
		}
#endif
	}

	err = sipsess_accept(&call->sess, sess_sock, msg, 180, "Ringing",
			     ua_cuser(call->ua), "application/sdp", NULL,
			     auth_handler, call->acc, true,
			     sipsess_offer_handler, sipsess_answer_handler,
			     sipsess_estab_handler, sipsess_info_handler,
			     sipsess_refer_handler, sipsess_close_handler,
			     call, "Allow: %s\r\n", uag_allowed_methods());
	if (err) {
		warning("call: sipsess_accept: %m\n", err);
		return err;
	}

	set_state(call, STATE_INCOMING);

	/* New call */
	tmr_start(&call->tmr_inv, LOCAL_TIMEOUT*1000, invite_timeout, call);

	if (!call->acc->mnat)
		call_event_handler(call, CALL_EVENT_INCOMING, call->peer_uri);

	return err;
}


static void sipsess_progr_handler(const struct sip_msg *msg, void *arg)
{
	struct call *call = arg;
	bool media;

	info("call: SIP Progress: %u %r (%r/%r)\n",
	     msg->scode, &msg->reason, &msg->ctyp.type, &msg->ctyp.subtype);

	if (msg->scode <= 100)
		return;

	/* check for 18x and content-type
	 *
	 * 1. start media-stream if application/sdp
	 * 2. play local ringback tone if not
	 *
	 * we must also handle changes to/from 180 and 183,
	 * so we reset the media-stream/ringback each time.
	 */
	if (msg_ctype_cmp(&msg->ctyp, "application", "sdp")
	    && mbuf_get_left(msg->mb)
	    && !sdp_decode(call->sdp, msg->mb, false)) {
		media = true;
	}
	else if (msg_ctype_cmp(&msg->ctyp, "multipart", "mixed") &&
		 !sdp_decode_multipart(&msg->ctyp.params, msg->mb) &&
		 !sdp_decode(call->sdp, msg->mb, false)) {
		media = true;
	}
	else
		media = false;

	switch (msg->scode) {

	case 180:
		set_state(call, STATE_RINGING);
		break;

	case 183:
		set_state(call, STATE_EARLY);
		break;
	}

	if (media)
		call_event_handler(call, CALL_EVENT_PROGRESS, call->peer_uri);
	else
		call_event_handler(call, CALL_EVENT_RINGING, call->peer_uri);

	call_stream_stop(call);

	if (media)
		call_stream_start(call, false);
}


static int send_invite(struct call *call)
{
	const char *routev[1];
	struct mbuf *desc;
	int err;

	routev[0] = ua_outbound(call->ua);

	err = call_sdp_get(call, &desc, true);
	if (err)
		return err;

	err = sipsess_connect(&call->sess, call->ua->owner->sock,
			      call->peer_uri,
			      call->local_name,
			      call->local_uri,
			      ua_cuser(call->ua),
			      routev[0] ? routev : NULL,
			      routev[0] ? 1 : 0,
			      "application/sdp", desc,
			      auth_handler, call->acc, true,
			      sipsess_offer_handler, sipsess_answer_handler,
			      sipsess_progr_handler, sipsess_estab_handler,
			      sipsess_info_handler, sipsess_refer_handler,
			      sipsess_close_handler, call,
			      "Allow: %s\r\n%H", uag_allowed_methods(),
			      ua_print_supported, call->ua);
	if (err) {
		warning("call: sipsess_connect: %m\n", err);
	}

	/* save call setup timer */
	call->time_conn = time(NULL);

	mem_deref(desc);

	return err;
}


/**
 * Get the current call duration in seconds
 *
 * @param call  Call object
 *
 * @return Duration in seconds
 */
uint32_t call_duration(const struct call *call)
{
	if (!call || !call->time_start)
		return 0;

	return (uint32_t)(time(NULL) - call->time_start);
}


/**
 * Get the current call setup time in seconds
 *
 * @param call  Call object
 *
 * @return Call setup in seconds
 */
/*
uint32_t call_setup_duration(const struct call *call)
{
	if (!call || !call->time_conn || call->time_conn <= 0 )
		return 0;

	return (uint32_t)(call->time_start - call->time_conn);
}
*/


/**
 * Get the audio object for the current call
 *
 * @param call  Call object
 *
 * @return Audio object
 */
struct audio *call_audio(const struct call *call)
{
	return call ? call->audio : NULL;
}


/**
 * Get the video object for the current call
 *
 * @param call  Call object
 *
 * @return Video object
 */
struct video *call_video(const struct call *call)
{
#ifdef USE_VIDEO
	return call ? call->video : NULL;
#else
	(void)call;
	return NULL;
#endif
}


/**
 * Get the list of media streams for the current call
 *
 * @param call  Call object
 *
 * @return List of media streams
 */
struct list *call_streaml(const struct call *call)
{
	return call ? (struct list *)&call->streaml : NULL;
}


int call_reset_transp(struct call *call)
{
	if (!call)
		return EINVAL;

	sdp_session_set_laddr(call->sdp, net_laddr_af(call->af));

	return call_modify(call);
}


int call_notify_sipfrag(struct call *call, uint16_t scode,
			const char *reason, ...)
{
	struct mbuf *mb;
	va_list ap;
	int err;

	if (!call)
		return EINVAL;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	va_start(ap, reason);
	(void)mbuf_printf(mb, "SIP/2.0 %u %v\n", scode, reason, &ap);
	va_end(ap);

	mb->pos = 0;

	if (scode >= 200) {
		err = sipevent_notify(call->not, mb, SIPEVENT_TERMINATED,
				      SIPEVENT_NORESOURCE, 0);

		call->not = mem_deref(call->not);
	}
	else {
		err = sipevent_notify(call->not, mb, SIPEVENT_ACTIVE,
				      SIPEVENT_NORESOURCE, 0);
	}

	mem_deref(mb);

	return err;
}


static void sipsub_notify_handler(struct sip *sip, const struct sip_msg *msg,
				  void *arg)
{
	struct call *call = arg;
	struct pl scode, reason;
	uint32_t sc;

	if (re_regex((char *)mbuf_buf(msg->mb), mbuf_get_left(msg->mb),
		     "SIP/2.0 [0-9]+ [^\r\n]+", &scode, &reason)) {
		(void)sip_reply(sip, msg, 400, "Bad sipfrag");
		return;
	}

	(void)sip_reply(sip, msg, 200, "OK");

	sc = pl_u32(&scode);

	if (sc >= 300) {
		warning("call: transfer failed: %u %r\n", sc, &reason);
		call_event_handler(call, CALL_EVENT_TRANSFER_FAILED,
				   "%u %r", sc, &reason);
	}
	else if (sc >= 200) {
		call_event_handler(call, CALL_EVENT_CLOSED, "Call transfered");
	}
}


static void sipsub_close_handler(int err, const struct sip_msg *msg,
				 const struct sipevent_substate *substate,
				 void *arg)
{
	struct call *call = arg;

	(void)substate;

	call->sub = mem_deref(call->sub);

	if (err) {
		info("call: subscription closed: %m\n", err);
	}
	else if (msg && msg->scode >= 300) {
		info("call: transfer failed: %u %r\n",
		     msg->scode, &msg->reason);
		call_event_handler(call, CALL_EVENT_TRANSFER_FAILED,
				   "%u %r", msg->scode, &msg->reason);
	}
}


static int normalize_uri(char **out, const char *uri, const struct uri *luri)
{
	struct uri uri2;
	struct pl pl;
	int err;

	if (!out || !uri || !luri)
		return EINVAL;

	pl_set_str(&pl, uri);

	if (0 == uri_decode(&uri2, &pl)) {

		err = str_dup(out, uri);
	}
	else {
		uri2 = *luri;

		uri2.user     = pl;
		uri2.password = pl_null;
		uri2.params   = pl_null;

		err = re_sdprintf(out, "%H", uri_encode, &uri2);
	}

	return err;
}


/**
 * Transfer the call to a target SIP uri
 *
 * @param call  Call object
 * @param uri   Target SIP uri
 *
 * @return 0 if success, otherwise errorcode
 */
int call_transfer(struct call *call, const char *uri)
{
	char *nuri;
	int err;

	if (!call || !uri)
		return EINVAL;

	err = normalize_uri(&nuri, uri, &call->acc->luri);
	if (err)
		return err;

	info("transferring call to %s\n", nuri);

	call->sub = mem_deref(call->sub);
	err = sipevent_drefer(&call->sub, call->ua->owner->evsock,
			      sipsess_dialog(call->sess), ua_cuser(call->ua),
			      auth_handler, call->acc, true,
			      sipsub_notify_handler, sipsub_close_handler,
			      call,
			      "Refer-To: %s\r\n", nuri);
	if (err) {
		warning("call: sipevent_drefer: %m\n", err);
	}

	mem_deref(nuri);

	return err;
}


int call_af(const struct call *call)
{
	return call ? call->af : AF_UNSPEC;
}


uint16_t call_scode(const struct call *call)
{
	return call ? call->scode : 0;
}


void call_set_handlers(struct call *call, call_event_h *eh,
		       call_dtmf_h *dtmfh, void *arg)
{
	if (!call)
		return;

	if (eh)
		call->eh    = eh;

	if (dtmfh)
		call->dtmfh = dtmfh;

	if (arg)
		call->arg   = arg;
}


bool call_is_onhold(const struct call *call)
{
	return call ? call->on_hold : false;
}


bool call_is_outgoing(const struct call *call)
{
	return call ? call->outgoing : false;
}

void call_connection_tx_audio(void* call, uint8_t *data, size_t len)
{
	audio_send(call_audio((struct call*)call), data, len);
}

void call_connection_tx_video(void* call, uint8_t *data, size_t len)
{
	video_send(call_video((struct call*)call), data, len);
}
void call_connection_fir(void* call)
{
	video_fir(call_video((struct call*)call));
}
