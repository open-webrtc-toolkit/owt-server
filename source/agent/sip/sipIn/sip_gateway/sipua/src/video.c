/**
 * @file src/video.c  Video stream
 *
 * Copyright (C) 2010 Creytiv.com
 *
 * \ref GenericVideoStream
 */
#include <string.h>
#include <stdlib.h>
#include <re/re.h>
#include <baresip.h>
#include "core.h"


/** Internal video-encoder format */
#ifndef VIDENC_INTERNAL_FMT
#define VIDENC_INTERNAL_FMT (VID_FMT_YUV420P)
#endif


enum {
	SRATE = 90000,
	MAX_MUTED_FRAMES = 3,
};

/** Video transmit parameters */
enum {
	MEDIA_POLL_RATE = 250,                 /**< in [Hz]             */
	BURST_MAX       = 8192,                /**< in bytes            */
	RTP_PRESZ       = 4 + RTP_HEADER_SIZE, /**< TURN and RTP header */
	RTP_TRAILSZ     = 12 + 4,              /**< SRTP/SRTCP trailer  */
};


/**
 * \page GenericVideoStream Generic Video Stream
 *
 * Implements a generic video stream. The application can allocate multiple
 * instances of a video stream, mapping it to a particular SDP media line.
 * The video object has a Video Display and Source, and a video encoder
 * and decoder. A particular video object is mapped to a generic media
 * stream object.
 *
 *<pre>
 *            recv  send
 *              |    /|\
 *             \|/    |
 *            .---------.    .-------.
 *            |  video  |--->|encoder|
 *            |         |    |-------|
 *            | object  |--->|decoder|
 *            '---------'    '-------'
 *              |    /|\
 *              |     |
 *             \|/    |
 *        .-------.  .-------.
 *        |Video  |  |Video  |
 *        |Display|  |Source |
 *        '-------'  '-------'
 *</pre>
 */

/**
 * Video stream - transmitter/encoder direction

 \verbatim

 Processing encoder pipeline:

 .         .--------.   .- - - - -.   .---------.   .---------.
 | ._O_.   |        |   !         !   |         |   |         |
 | |___|-->| vidsrc |-->! vidconv !-->| vidfilt |-->| encoder |---> RTP
 |         |        |   !         !   |         |   |         |
 '         '--------'   '- - - - -'   '---------'   '---------'
                         (optional)
 \endverbatim
 */
struct vtx {
	struct video *video;               /**< Parent                    */
	const struct vidcodec *vc;         /**< Current Video encoder     */
	struct vidframe *frame;            /**< Source frame              */
        struct mbuf *mb;                   /**< Packetization buffer      */
	// TODO struct list sendq;                 /**< Tx-Queue (struct vidqent) */
	struct tmr tmr_rtp;                /**< Timer for sending RTP     */
	int muted_frames;                  /**< # of muted frames sent    */
	uint32_t ts_tx;                    /**< Outgoing RTP timestamp    */
	bool picup;                        /**< Send picture update       */
	bool muted;                        /**< Muted flag                */
	int frames;                        /**< Number of frames sent     */
	int efps;                          /**< Estimated frame-rate      */
};



/**
 * Video stream - receiver/decoder direction

 \verbatim

 Processing decoder pipeline:

 .~~~~~~~~.   .--------.   .---------.   .---------.
 |  _o_   |   |        |   |         |   |         |
 |   |    |<--| vidisp |<--| vidfilt |<--| decoder |<--- RTP
 |  /'\   |   |        |   |         |   |         |
 '~~~~~~~~'   '--------'   '---------'   '---------'

 \endverbatim

 */
struct vrx {
	struct video *video;               /**< Parent                    */
	const struct vidcodec *vc;         /**< Current video decoder     */
	int pt_rx;                         /**< Incoming RTP payload type */
	int frames;                        /**< Number of frames received */
	int efps;                          /**< Estimated frame-rate      */
    int rx_counter;                    /**< counter for handling connection timeout*/
};


/** Generic Video stream */
struct video {
	struct call* call; /* the call of current stream */
	struct config_video cfg;/**< Video configuration                  */
	struct stream *strm;    /**< Generic media stream                 */
	struct vtx vtx;         /**< Transmit/encoder direction           */
	struct vrx vrx;         /**< Receive/decoder direction            */
	struct tmr tmr;         /**< Timer for frame-rate estimation      */
	char *peer;             /**< Peer URI                             */
	bool nack_pli;          /**< Send NACK/PLI to peer                */
	video_err_h *errh;
	void *arg;
};

#if 0
struct vidqent {
	struct le le;
	struct sa dst;
	bool marker;
	uint8_t pt;
	uint32_t ts;
	struct mbuf *mb;
};


static void vidqent_destructor(void *arg)
{
	struct vidqent *qent = arg;

	list_unlink(&qent->le);
	mem_deref(qent->mb);
}


static int vidqent_alloc(struct vidqent **qentp,
			 bool marker, uint8_t pt, uint32_t ts,
			 const uint8_t *hdr, size_t hdr_len,
			 const uint8_t *pld, size_t pld_len)
{
	struct vidqent *qent;
	int err = 0;

	if (!qentp || !pld)
		return EINVAL;

	qent = mem_zalloc(sizeof(*qent), vidqent_destructor);
	if (!qent)
		return ENOMEM;

	qent->marker = marker;
	qent->pt     = pt;
	qent->ts     = ts;

	qent->mb = mbuf_alloc(RTP_PRESZ + hdr_len + pld_len + RTP_TRAILSZ);
	if (!qent->mb) {
		err = ENOMEM;
		goto out;
	}

	qent->mb->pos = qent->mb->end = RTP_PRESZ;

	if (hdr)
		(void)mbuf_write_mem(qent->mb, hdr, hdr_len);

	(void)mbuf_write_mem(qent->mb, pld, pld_len);

	qent->mb->pos = RTP_PRESZ;

 out:
	if (err)
		mem_deref(qent);
	else
		*qentp = qent;

	return err;
}

static void vidqueue_poll(struct vtx *vtx, uint64_t jfs, uint64_t prev_jfs)
{
	size_t burst, sent;
	uint64_t bandwidth_kbps;
	struct le *le;

	if (!vtx)
		return;


	le = vtx->sendq.head;
	if (!le)
                return;

	/*
	 * time [ms] * bitrate [kbps] / 8 = bytes
	 */
	bandwidth_kbps = vtx->video->cfg.bitrate / 1000;
	burst = (1 + jfs - prev_jfs) * bandwidth_kbps / 4;

	burst = min(burst, BURST_MAX);
	sent  = 0;

	while (le) {

		struct vidqent *qent = le->data;

		sent += mbuf_get_left(qent->mb);

		stream_send(vtx->video->strm, qent->marker, qent->pt,
			    qent->ts, qent->mb);

		le = le->next;
		mem_deref(qent);

		if (sent > burst) {
			break;
		}
	}

}
#endif

/*
static void rtp_tmr_handler(void *arg)
{
	struct vtx *vtx = arg;
	uint64_t pjfs;

	pjfs = vtx->tmr_rtp.jfs;

	tmr_start(&vtx->tmr_rtp, 1000/MEDIA_POLL_RATE, rtp_tmr_handler, vtx);

	vidqueue_poll(vtx, vtx->tmr_rtp.jfs, pjfs);
}
*/


static void video_destructor(void *arg)
{
	struct video *v = arg;
	struct vtx *vtx = &v->vtx;

	/* transmit */
	// list_flush(&vtx->sendq);

	// tmr_cancel(&vtx->tmr_rtp);
	mem_deref(vtx->frame);
        mem_deref(vtx->mb);

	/* receive */
	tmr_cancel(&v->tmr);
	mem_deref(v->strm);
	mem_deref(v->peer);
}

extern void call_connection_rx_fir(void *owner);
extern void call_connection_rx_gnack(void* owner, uint32_t ssrcPacket, uint32_t ssrcMedia, uint32_t n, uint32_t* pid_blp);
extern void ep_update_video_params(void *gateway, const char *peer, const char *cdcname, int bitrate, int packetsize, int fps, const char *fmtp);

static int get_fps(const struct video *v)
{
	const char *attr;

	/* RFC4566 */
	attr = sdp_media_rattr(stream_sdpmedia(v->strm), "framerate");
	if (attr) {
		/* NOTE: fractional values are ignored */
		const double fps = atof(attr);
		return (int)fps;
	}
	else
		return v->cfg.fps;
}


static int vtx_alloc(struct vtx *vtx, struct video *video)
{

	// tmr_init(&vtx->tmr_rtp);
        vtx->mb = mbuf_alloc(STREAM_PRESZ + 512);
	if (!vtx->mb)
		return ENOMEM;

	vtx->video = video;
	vtx->ts_tx = 160;

	// tmr_start(&vtx->tmr_rtp, 1, rtp_tmr_handler, vtx);

	return 0;
}


static int vrx_alloc(struct vrx *vrx, struct video *video)
{

	vrx->video  = video;
	vrx->pt_rx  = -1;

	return 0;
}


/**
 * Decode incoming RTP packets using the Video decoder
 *
 * NOTE: mb=NULL if no packet received
 *
 * @param vrx Video receive object
 * @param hdr RTP Header
 * @param mb  Buffer with RTP payload
 *
 * @return 0 if success, otherwise errorcode
 */
 /* TODO intel webrtc */
#if 0
static int video_stream_decode(struct vrx *vrx, const struct rtp_header *hdr,
			       struct mbuf *mb)
{
	struct video *v = vrx->video;
	struct vidframe frame;
	struct le *le;
	int err = 0;

	if (!hdr || !mbuf_get_left(mb))
		return 0;

	lock_write_get(vrx->lock);

	/* No decoder set */
	if (!vrx->dec) {
		warning("video: No video decoder!\n");
		goto out;
	}

	frame.data[0] = NULL;
	err = vrx->vc->dech(vrx->dec, &frame, hdr->m, hdr->seq, mb);
	if (err) {

		if (err != EPROTO) {
			warning("video: %s decode error"
				" (seq=%u, %u bytes): %m\n",
				vrx->vc->name, hdr->seq,
				mbuf_get_left(mb), err);
		}

		/* send RTCP FIR to peer */
		stream_send_fir(v->strm, v->nack_pli);

		/* XXX: if RTCP is not enabled, send XML in SIP INFO ? */

		goto out;
	}

	/* Got a full picture-frame? */
	if (!vidframe_isvalid(&frame))
		goto out;

	/* Process video frame through all Video Filters */
	for (le = vrx->filtl.head; le; le = le->next) {

		struct vidfilt_dec_st *st = le->data;

		if (st->vf && st->vf->dech)
			err |= st->vf->dech(st, &frame);
	}

	err = vidisp_display(vrx->vidisp, v->peer, &frame);
	if (err == ENODEV) {
		warning("video: video-display was closed\n");
		vrx->vidisp = mem_deref(vrx->vidisp);

		lock_rel(vrx->lock);

		if (v->errh) {
			v->errh(err, "display closed", v->arg);
		}

		return err;
	}

	++vrx->frames;

out:
	lock_rel(vrx->lock);

	return err;
}
#endif


static int pt_handler(struct video *v, uint8_t pt_old, uint8_t pt_new)
{
	/* const struct sdp_format *lc;

	lc = sdp_media_lformat(stream_sdpmedia(v->strm), pt_new);
	if (!lc)
		return ENOENT;
	*/

	if (pt_old != (uint8_t)-1) {
		info("Video decoder changed payload %u -> %u\n",
		     pt_old, pt_new);
	}

	v->vrx.pt_rx = pt_new;

	/* return video_decoder_set(v, lc->data, lc->pt, lc->rparams); */
	return 0;
}

extern void call_connection_rx_video(void *owner, uint8_t *data, size_t len);

/* Handle incoming stream data from the network */
static void stream_recv_handler(const struct rtp_header *hdr,
				struct mbuf *mb, void *arg)
{
	struct video *v = arg;
	int err;
        void* call_owner = (v->call ? call_get_owner(v->call) : NULL);

	if (!mb)
		return;

	/* Video payload-type changed? */
	if (hdr->pt != v->vrx.pt_rx) {
		err = pt_handler(v, v->vrx.pt_rx, hdr->pt);
		if (err)
			return;
	}

	mb->pos = 0;
    if (mbuf_get_left(mb) && v->call && call_owner) {
        ++v->vrx.rx_counter;
        call_connection_rx_video(call_owner, mbuf_buf(mb), mbuf_get_left(mb));
    }
}


static void rtcp_handler(struct rtcp_msg *msg, void *arg)
{
	struct video *v = arg;
  uint32_t fci[32] = {0};
  size_t i = 0;
	void *owner = (v->call ? call_get_owner(v->call) : NULL);
	if (!owner)
		return;

	switch (msg->hdr.pt) {

	case RTCP_FIR:
		v->vtx.picup = true;
		call_connection_rx_fir(owner);
		break;

	case RTCP_PSFB:
		if (msg->hdr.count == RTCP_PSFB_PLI) {
			v->vtx.picup = true;
			call_connection_rx_fir(owner);
		}
		break;

	case RTCP_RTPFB:
		if (msg->hdr.count == RTCP_RTPFB_GNACK) {
			v->vtx.picup = true;
      if (msg->r.fb.n > 32) {
        info("Too manay GNACK blocks:%u", msg->r.fb.n);
        break;
      }
      for (i = 0; i < msg->r.fb.n; i++) {
        fci[i] = msg->r.fb.fci.gnackv[i].pid;
        fci[i] = (fci[i] << 16) + msg->r.fb.fci.gnackv[i].blp;
      }
			call_connection_rx_gnack(owner, msg->r.fb.ssrc_packet, msg->r.fb.ssrc_media, msg->r.fb.n, fci);
		}
		break;

	default:
		break;
	}
}


int video_alloc(struct video **vp, const struct config *cfg,
		struct call *call, struct sdp_session *sdp_sess, int label,
		const struct mnat *mnat, struct mnat_sess *mnat_sess,
		const struct menc *menc, struct menc_sess *menc_sess,
		const char *content, const struct list *vidcodecl,
		video_err_h *errh, void *arg)
{
	struct video *v;
	struct le *le;
	int err = 0;

	if (!vp || !cfg)
		return EINVAL;

	v = mem_zalloc(sizeof(*v), video_destructor);
	if (!v)
		return ENOMEM;

	v->call = call;

	v->cfg = cfg->video;
	tmr_init(&v->tmr);

	err = stream_alloc(&v->strm, &cfg->avt, call, sdp_sess, "video", label,
			   mnat, mnat_sess, menc, menc_sess,
			   call_localuri(call),
			   stream_recv_handler, rtcp_handler, v);
	if (err)
		goto out;

	if (cfg->avt.rtp_bw.max >= AUDIO_BANDWIDTH) {
		stream_set_bw(v->strm, cfg->avt.rtp_bw.max - AUDIO_BANDWIDTH);
	}

	err |= sdp_media_set_lattr(stream_sdpmedia(v->strm), true,
				   "framerate", "%d", v->cfg.fps);

	/* RFC 4585 */
	err |= sdp_media_set_lattr(stream_sdpmedia(v->strm), true,
				   "rtcp-fb", "* nack");

	/* RFC 5104 */
	err |= sdp_media_set_lattr(stream_sdpmedia(v->strm), false,
				   "rtcp-fb", "* ccm fir");

	/* RFC 4796 */
	if (content) {
		err |= sdp_media_set_lattr(stream_sdpmedia(v->strm), true,
					   "content", "%s", content);
	}

	if (err)
		goto out;

	v->errh = errh;
	v->arg = arg;

	err  = vtx_alloc(&v->vtx, v);
	err |= vrx_alloc(&v->vrx, v);
	if (err)
		goto out;

	/* Video codecs */
	for (le = list_head(vidcodecl); le; le = le->next) {
		struct vidcodec *vc = le->data;
		err |= sdp_format_add(NULL, stream_sdpmedia(v->strm), false,
				      vc->pt, vc->name, 90000, 1,
				      vc->fmtp_ench, vc->fmtp_cmph, vc, false,
				      "%s", vc->fmtp);
	}

 out:
	if (err)
		mem_deref(v);
	else
		*vp = v;

	return err;
}


/* Set the encoder format - can be called multiple times */
/* TODO intel webrtc */
/*
static int set_encoder_format(struct vtx *vtx, struct vidsz *size)
{
	int err;

	vtx->mute_frame = mem_deref(vtx->mute_frame);
	err = vidframe_alloc(&vtx->mute_frame, VIDENC_INTERNAL_FMT, size);
	if (err)
		return err;

	vidframe_fill(vtx->mute_frame, 0xff, 0xff, 0xff);

	return err;
}
*/


enum {TMR_INTERVAL = 5};
static void tmr_handler(void *arg)
{
	struct video *v = arg;

	tmr_start(&v->tmr, TMR_INTERVAL * 1000, tmr_handler, v);

	/* Estimate framerates */
	v->vtx.efps = v->vtx.frames / TMR_INTERVAL;
	v->vrx.efps = v->vrx.frames / TMR_INTERVAL;

	v->vtx.frames = 0;
	v->vrx.frames = 0;
}


int video_start(struct video *v, const char *peer)
{
	int err;

	if (!v)
		return EINVAL;

	if (peer) {
		mem_deref(v->peer);
		err = str_dup(&v->peer, peer);
		if (err)
			return err;
	}

	stream_set_srate(v->strm, SRATE, SRATE);

	/* TODO intel webrtc */
	/*
	size.w = v->cfg.width;
	size.h = v->cfg.height;
	err = set_encoder_format(&v->vtx, v->cfg.src_mod,
				 v->vtx.device, &size);
	if (err) {
		warning("video: could not set encoder format to"
			" [%u x %u] %m\n",
			size.w, size.h, err);
	}
	*/

	tmr_start(&v->tmr, TMR_INTERVAL * 1000, tmr_handler, v);
	return 0;
}


void video_stop(struct video *v)
{
	if (!v)
		return;
}


static void settledown_video_codec(struct sdp_media *m, const char *name)
{
	struct sdp_format *lfmt;
	struct le *lle;
	const struct list *lfmtl = sdp_media_format_lst(m, true);

	for (lle=lfmtl->tail; lle; ) {
		lfmt = lle->data;
		lle = lle->prev;

		if (str_casecmp(lfmt->name, name)) {
			lfmt->rparams = mem_deref(lfmt->rparams);
			list_unlink(&lfmt->le);
			mem_deref(lfmt);
		}
	}
}


/**
 * Mute the video stream
 *
 * @param v     Video stream
 * @param muted True to mute, false to un-mute
 */
// TODO intel webrtc
void video_mute(struct video *v, bool muted)
{
	struct vtx *vtx;

	if (!v)
		return;

	vtx = &v->vtx;

	vtx->muted        = muted;
	vtx->muted_frames = 0;
	vtx->picup        = true;
	// TODO
}



int video_encoder_set(struct video *v, struct vidcodec *vc,
		      int pt_tx, const char *params)
{
	struct vtx *vtx;
	int err = 0;

	if (!v)
		return EINVAL;

	vtx = &v->vtx;

	if (vc != vtx->vc && call_get_ua(v->call) && call_peeruri(v->call)) {
		info("Set video encoder: %s %s (%u bit/s, %u fps)\n",
		     vc->name, vc->variant, v->cfg.bitrate, get_fps(v));

		vtx->vc = vc;
		ep_update_video_params(call_get_ua(v->call)->owner->ep, call_peeruri(v->call), vc->name, v->cfg.bitrate, 1024, get_fps(v), params);
		settledown_video_codec(stream_sdpmedia(v->strm),  vc->name);
	}

	stream_update_encoder(v->strm, pt_tx);

	return err;
}


int video_decoder_set(struct video *v, struct vidcodec *vc, int pt_rx,
		      const char *fmtp)
{
	struct vrx *vrx;
	int err = 0;
	(void)fmtp;

	if (!v)
		return EINVAL;

	/* handle vidcodecs without a decoder */
        /* TODO intel webrtc */
        /*
	if (!vc->decupdh) {
		struct vidcodec *vcd;

		info("video: vidcodec '%s' has no decoder\n", vc->name);

		vcd = (struct vidcodec *)vidcodec_find_decoder(vc->name);
		if (!vcd) {
			warning("video: could not find decoder (%s)\n",
				vc->name);
			return ENOENT;
		}

		vc = vcd;
	} */

	vrx = &v->vrx;

	vrx->pt_rx = pt_rx;

	if (vc != vrx->vc) {
		info("Set video decoder: %s %s\n", vc->name, vc->variant);
		vrx->vc = vc;
	}

	return err;
}

int get_video_counter(const struct video *video){
    return video ? video->vrx.rx_counter : 0;
}

void reset_video_counter(struct video *video){
    if(video){
        video->vrx.rx_counter = 0;
    }
}

struct stream *video_strm(const struct video *v)
{
	return v ? v->strm : NULL;
}


void video_update_picture(struct video *v)
{
	if (!v)
		return;
	v->vtx.picup = true;
}


static bool sdprattr_contains(struct stream *s, const char *name,
			      const char *str)
{
	const char *attr = sdp_media_rattr(stream_sdpmedia(s), name);
	return attr ? (NULL != strstr(attr, str)) : false;
}


void video_sdp_attr_decode(struct video *v)
{
	if (!v)
		return;

	/* RFC 4585 */
	v->nack_pli = sdprattr_contains(v->strm, "rtcp-fb", "nack");
}


int video_print(struct re_printf *pf, const struct video *v)
{
	if (!v)
		return 0;

	return re_hprintf(pf, " efps=%d/%d", v->vtx.efps, v->vrx.efps);
}

static int pre_decode_rtpheader(struct rtp_header *hdr, uint8_t *buf, size_t len)
{
	int i;
	size_t header_len;

	if (!hdr || !buf)
		return -1;

	if (len < RTP_HEADER_SIZE)
		return -2;

	hdr->ver  = (buf[0] >> 6) & 0x03;
	hdr->pad  = (buf[0] >> 5) & 0x01;
	hdr->ext  = (buf[0] >> 4) & 0x01;
	hdr->cc   = (buf[0] >> 0) & 0x0f;
	hdr->m    = (buf[1] >> 7) & 0x01;
	hdr->pt   = (buf[1] >> 0) & 0x7f;

	hdr->seq  = ntohs(*(uint16_t*)(&buf[2]));
	hdr->ts   = ntohl(*(uint32_t*)(&buf[4]));
	hdr->ssrc = ntohl(*(uint32_t*)(&buf[8]));

	header_len = hdr->cc*sizeof(uint32_t);
	if ((len - RTP_HEADER_SIZE) < header_len)
		return -3;

	for (i=0; i<hdr->cc; i++) {
		hdr->csrc[i] = ntohl(*(uint32_t*)(&buf[RTP_HEADER_SIZE+i]));
	}

	if (hdr->ext) {
		if ((len - RTP_HEADER_SIZE - header_len) < 4)
			return -4;

		hdr->x.type = ntohs(*(uint16_t*)(&buf[RTP_HEADER_SIZE+header_len]));
		hdr->x.len  = ntohs(*(uint16_t*)(&buf[RTP_HEADER_SIZE+header_len+2]));

		if ((len - RTP_HEADER_SIZE - header_len - 4) < hdr->x.len*sizeof(uint32_t))
			return -5;
	}

	return 0;
}

/* TODO intel webrtc send data.*/
void video_send(struct video *v, uint8_t *data, size_t len)
{
	struct rtp_header hdr;
	struct vtx *vtx = NULL;
	int err, pl_pos, pl_len;

        if (!v) {
		/*warning("video_send: v == NULL.\n");*/
		goto out;
	}

	if (len < RTP_HEADER_SIZE) {
		warning("audio_send: len < 12\n");
		goto out;
	}

        err = pre_decode_rtpheader(&hdr, data, len);
        if (err) {
    		warning("video_send: rtp_hdr_decode failed: err=%d.\n", err);
    		goto out;
    	}
    
    	vtx = &v->vtx;
        vtx->mb->pos = vtx->mb->end = STREAM_PRESZ;
        pl_pos = hdr.ext ? RTP_HEADER_SIZE + 4 * hdr.cc + 4 + hdr.x.len : RTP_HEADER_SIZE + 4 * hdr.cc;
        pl_len = len - pl_pos;
    
        err = mbuf_write_mem(vtx->mb, data + pl_pos , pl_len);
    	if (err) {
    		warning("video_send: mbuf_write_mem failed.\n");
    		goto out;
    	}
    
        vtx->mb->pos = STREAM_PRESZ;
        vtx->mb->end = STREAM_PRESZ + pl_len;
        err = stream_send(v->strm, hdr.m, -1, hdr.ts, vtx->mb);
        if (err){
        	warning("video_send: stream_send failed.\n");
        	goto out;
        }

 out:
    return;
}


void video_fir(struct video *v)
{
	/* send RTCP FIR to peer */
	if (v) {
	    stream_send_fir(v->strm, v->nack_pli);
	}
}
