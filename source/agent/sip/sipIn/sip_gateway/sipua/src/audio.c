/**
 * @file src/audio.c  Audio stream
 *
 * Copyright (C) 2010 Creytiv.com
 * \ref GenericAudioStream
 */
#define _DEFAULT_SOURCE 1
#define _BSD_SOURCE 1
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <re.h>
#include <baresip.h>
#include "core.h"


/**
 * \page GenericAudioStream Generic Audio Stream
 *
 * Implements a generic audio stream. The application can allocate multiple
 * instances of a audio stream, mapping it to a particular SDP media line.
 * The audio object has a DSP sound card sink and source, and an audio encoder
 * and decoder. A particular audio object is mapped to a generic media
 * stream object. Each audio channel has an optional audio filtering chain.
 *
 *<pre>
 *            write  read
 *              |    /|\
 *             \|/    |
 * .------.   .---------.    .-------.
 * |filter|<--|  audio  |--->|encoder|
 * '------'   |         |    |-------|
 *            | object  |--->|decoder|
 *            '---------'    '-------'
 *              |    /|\
 *              |     |
 *             \|/    |
 *         .------. .-----.
 *         |auplay| |ausrc|
 *         '------' '-----'
 *</pre>
 */

enum {
	AUDIO_SAMPSZ    = 3*1920  /* Max samples, 48000Hz 2ch at 60ms */
};


/**
 * Audio transmit/encoder
 *
 *
 \verbatim

 Processing encoder pipeline:

 .    .-------.   .-------.   .--------.   .--------.   .--------.
 |    |       |   |       |   |        |   |        |   |        |
 |O-->| ausrc |-->| aubuf |-->| resamp |-->| aufilt |-->| encode |---> RTP
 |    |       |   |       |   |        |   |        |   |        |
 '    '-------'   '-------'   '--------'   '--------'   '--------'

 \endverbatim
 *
 */

struct autx {
	const struct aucodec *ac;     /**< Current audio encoder           */
	struct mbuf *mb;              /**< Buffer for outgoing RTP packets */
	uint32_t ptime;               /**< Packet time for sending         */
	uint32_t ts;                  /**< Timestamp for outgoing RTP      */
	uint32_t ts_tel;              /**< Timestamp for Telephony Events  */
	size_t psize;                 /**< Packet size for sending         */
	bool marker;                  /**< Marker bit for outgoing RTP     */
	bool is_g722;                 /**< Set if encoder is G.722 codec   */
	int cur_key;                  /**< Currently transmitted event     */
};


/**
 * Audio receive/decoder
 *
 \verbatim

 Processing decoder pipeline:

       .--------.   .-------.   .--------.   .--------.   .--------.
 |\    |        |   |       |   |        |   |        |   |        |
 | |<--| auplay |<--| aubuf |<--| resamp |<--| aufilt |<--| decode |<--- RTP
 |/    |        |   |       |   |        |   |        |   |        |
       '--------'   '-------'   '--------'   '--------'   '--------'

 \endverbatim
 */
struct aurx {
	const struct aucodec *ac;     /**< Current audio decoder           */
	uint32_t ptime;               /**< Packet time for receiving       */
	int pt;                       /**< Payload type for incoming RTP   */
	int pt_tel;                   /**< Event payload type - receive    */
    int rx_counter;               /**< counter for handling connection timeout*/
};


/** Generic Audio stream */
struct audio {
	// MAGIC_DECL                    /**< Magic number for debugging      */
	struct call *call;   /* the call of the audio stream */
	struct autx tx;               /**< Transmit                        */
	struct aurx rx;               /**< Receive                         */
	struct stream *strm;          /**< Generic media stream            */
	struct telev *telev;          /**< Telephony events                */
	struct config_audio cfg;      /**< Audio configuration             */
	bool started;                 /**< Stream is started flag          */
	audio_event_h *eventh;        /**< Event handler                   */
	audio_err_h *errh;            /**< Audio error handler             */
	void *arg;                    /**< Handler argument                */
};

int get_audio_counter(const struct audio *audio){
    return audio ? audio->rx.rx_counter : 0;
}

void reset_audio_counter(struct audio *audio){
    if(audio){
        audio->rx.rx_counter = 0;
    }
}

static void stop_tx(struct autx *tx, struct audio *a)
{
	if (!tx || !a)
		return;
}


static void stop_rx(struct aurx *rx)
{
	if (!rx)
		return;
}


static void audio_destructor(void *arg)
{
	struct audio *a = arg;

	stop_tx(&a->tx, a);
	stop_rx(&a->rx);

	mem_deref(a->tx.mb);

	mem_deref(a->strm);
	mem_deref(a->telev);
}


/**
 * Calculate number of samples from sample rate, channels and packet time
 *
 * @param srate    Sample rate in [Hz]
 * @param channels Number of channels
 * @param ptime    Packet time in [ms]
 *
 * @return Number of samples
 */
static inline uint32_t calc_nsamp(uint32_t srate, uint8_t channels,
				  uint16_t ptime)
{
	return srate * channels * ptime / 1000;
}


/**
 * Get the DSP samplerate for an audio-codec (exception for G.722 and MPA)
 */
static inline uint32_t get_srate(const struct aucodec *ac)
{
	if (!ac)
		return 0;

	return ac->srate;
}


/**
 * Get the DSP channels for an audio-codec (exception for MPA)
 */
static inline uint32_t get_ch(const struct aucodec *ac)
{
	if (!ac)
		return 0;

	return !str_casecmp(ac->name, "MPA") ? 2 : ac->ch;
}


static inline uint32_t get_framesize(const struct aucodec *ac,
				     uint32_t ptime)
{
	if (!ac)
		return 0;

	return calc_nsamp(get_srate(ac), get_ch(ac), ptime);
}


static bool aucodec_equal(const struct aucodec *a, const struct aucodec *b)
{
	if (!a || !b)
		return false;

	return get_srate(a) == get_srate(b) && get_ch(a) == get_ch(b);
}


static int add_audio_codec(struct audio *a, struct sdp_media *m,
			   struct aucodec *ac)
{
	if (!in_range(&a->cfg.srate, get_srate(ac))) {
		debug("audio: skip %uHz codec (audio range %uHz - %uHz)\n",
		      get_srate(ac), a->cfg.srate.min, a->cfg.srate.max);
		return 0;
	}

	if (!in_range(&a->cfg.channels, get_ch(ac))) {
		debug("audio: skip codec with %uch (audio range %uch-%uch)\n",
		      get_ch(ac), a->cfg.channels.min, a->cfg.channels.max);
		return 0;
	}

	/* intel webrtc 
        if (ac->crate < 8000) {
		warning("audio: illegal clock rate %u\n", ac->crate);
		return EINVAL;
	}
        */

	return sdp_format_add(NULL, m, false, ac->pt, ac->name, ac->srate,
			      ac->ch, ac->fmtp_ench, ac->fmtp_cmph, ac, false,
			      "%s", ac->fmtp);
}


static void settledown_audio_codec(struct sdp_media *m, const char *name,
		   uint32_t srate, uint8_t ch)
{
    struct sdp_format *lfmt;
    struct le *lle;
    const struct list *lfmtl = sdp_media_format_lst(m, true);

    for (lle=lfmtl->tail; lle; ) {
        lfmt = lle->data;
        lle = lle->prev;

        if (str_casecmp(lfmt->name, telev_rtpfmt) &&
                (str_casecmp(lfmt->name, name) || (lfmt->srate != srate) || (lfmt->ch != ch))) {
            lfmt->rparams = mem_deref(lfmt->rparams);
            list_unlink(&lfmt->le);
            mem_deref(lfmt);
        }
    }
}

#if 0
/**
 * Encoder audio and send via stream
 *
 * @note This function has REAL-TIME properties
 *
 * @param a     Audio object
 * @param tx    Audio transmit object
 * @param sampv Audio samples
 * @param sampc Number of audio samples
 */
static void encode_rtp_send(struct audio *a, struct autx *tx,
			    int16_t *sampv, size_t sampc)
{
	size_t sampc_rtp;
	size_t len;
	int err;

	if (!tx->ac)
		return;

	tx->mb->pos = tx->mb->end = STREAM_PRESZ;
	len = mbuf_get_space(tx->mb);

	err = tx->ac->ench(tx->enc, mbuf_buf(tx->mb), &len, sampv, sampc);
	if ((err & 0xffff0000) == 0x00010000) {
		/* MPA needs some special treatment here */
		tx->ts = err & 0xffff;
		err = 0;
	}
	else if (err) {
		warning("audio: %s encode error: %d samples (%m)\n",
			tx->ac->name, sampc, err);
		goto out;
	}

	tx->mb->pos = STREAM_PRESZ;
	tx->mb->end = STREAM_PRESZ + len;

	if (mbuf_get_left(tx->mb)) {
		if (len) {
			err = stream_send(a->strm, tx->marker, -1,
					tx->ts, tx->mb);
			if (err)
				goto out;
		}
	}

	/* Convert from audio samplerate to RTP clockrate */
	sampc_rtp = sampc * tx->ac->crate / tx->ac->srate;

	/* The RTP clock rate used for generating the RTP timestamp is
	 * independent of the number of channels and the encoding
	 * However, MPA support variable packet durations. Thus, MPA
	 * should update the ts according to its current internal state.
	 */
	frame_size = sampc_rtp / get_ch(tx->ac);

	tx->ts += (uint32_t)frame_size;

 out:
	tx->marker = false;
}

/*
 * @note This function has REAL-TIME properties
 */
static void poll_aubuf_tx(struct audio *a)
{
	struct autx *tx = &a->tx;
	int16_t *sampv = tx->sampv;
	size_t sampc;
	struct le *le;
	int err = 0;

	sampc = tx->psize / 2;

	/* timed read from audio-buffer */
	aubuf_read_samp(tx->aubuf, tx->sampv, sampc);

	/* optional resampler */
	if (tx->resamp.resample) {
		size_t sampc_rs = AUDIO_SAMPSZ;

		err = auresamp(&tx->resamp,
			       tx->sampv_rs, &sampc_rs,
			       tx->sampv, sampc);
		if (err)
			return;

		sampv = tx->sampv_rs;
		sampc = sampc_rs;
	}

	/* Process exactly one audio-frame in list order */
	for (le = tx->filtl.head; le; le = le->next) {
		struct aufilt_enc_st *st = le->data;

		if (st->af && st->af->ench)
			err |= st->af->ench(st, sampv, &sampc);
	}
	if (err) {
		warning("audio: aufilter encode: %m\n", err);
	}

	/* Encode and send */
	encode_rtp_send(a, tx, sampv, sampc);
}

static void check_telev(struct audio *a, struct autx *tx)
{
	const struct sdp_format *fmt;
	bool marker = false;
	int err;

	tx->mb->pos = tx->mb->end = STREAM_PRESZ;

	err = telev_poll(a->telev, &marker, tx->mb);
	if (err)
		return;

	if (marker)
		tx->ts_tel = tx->ts;

	fmt = sdp_media_rformat(stream_sdpmedia(audio_strm(a)), telev_rtpfmt);
	if (!fmt)
		return;

	tx->mb->pos = STREAM_PRESZ;
	err = stream_send(a->strm, marker, fmt->pt, tx->ts_tel, tx->mb);
	if (err) {
		warning("audio: telev: stream_send %m\n", err);
	}
}

#endif

static int pt_handler(struct audio *a, uint8_t pt_old, uint8_t pt_new)
{
	const struct sdp_format *lc;

	lc = sdp_media_lformat(stream_sdpmedia(a->strm), pt_new);
	if (!lc)
		return ENOENT;

	if (pt_old != (uint8_t)-1) {
		info("Audio decoder changed payload %u -> %u\n",
		     pt_old, pt_new);
	}

	a->rx.pt = pt_new;

	return audio_decoder_set(a, lc->data, lc->pt, lc->params);
}

static void handle_telev(struct audio *a, struct mbuf *mb)
{
	int event, digit;
	bool end;

	if (telev_recv(a->telev, mb, &event, &end))
		return;

	digit = telev_code2digit(event);
	if (digit >= 0 && a->eventh)
		a->eventh(digit, end, a->arg);
}

extern void call_connection_rx_audio(void *owner, uint8_t *data, size_t len);

/* Handle incoming stream data from the network */
static void stream_recv_handler(const struct rtp_header *hdr,
				struct mbuf *mb, void *arg)
{
	struct audio *a = arg;
	struct aurx *rx = &a->rx;
    void *owner = 0;


	/* Telephone event? */
    if (hdr->pt != rx->pt) {
        const struct sdp_format *fmt;

        fmt = sdp_media_lformat(stream_sdpmedia(a->strm), hdr->pt);

        if (fmt && !str_casecmp(fmt->name, telev_rtpfmt)) {
            handle_telev(a, mb);
            return;
        }
    }

	/* Comfort Noise (CN) as of RFC 3389 */
	if (PT_CN == hdr->pt)
		return;

	/* Audio payload-type changed? */
	/* XXX: this logic should be moved to stream.c */
	if (hdr->pt != rx->pt) {
		if (pt_handler(a, rx->pt, hdr->pt))
			return;
		return;
	}
	
	if (!mb) {
		return;
	}

    /***  Do NOT decode the stream data here, but send to transcoder ***/
    // (void)aurx_stream_decode(&a->rx, mb);
    mb->pos = 0;
    owner = call_get_owner(a->call);
    if (mbuf_get_left(mb) && owner) {
        ++a->rx.rx_counter;
        call_connection_rx_audio(owner, mbuf_buf(mb), mbuf_get_left(mb));
    }

    return;
}


static int add_telev_codec(struct audio *a)
{
	struct sdp_media *m = stream_sdpmedia(audio_strm(a));
	struct sdp_format *sf;
	int err;

	/* Use payload-type 101 if available, for CiscoGW interop */
	err = sdp_format_add(&sf, m, false,
			     (!sdp_media_lformat(m, 101)) ? "101" : NULL,
			     telev_rtpfmt, TELEV_SRATE, 1, NULL,
			     NULL, NULL, false, "0-15");
	if (err)
		return err;

	return err;
}


int audio_alloc(struct audio **ap, const struct config *cfg,
		struct call *call, struct sdp_session *sdp_sess, int label,
		const struct mnat *mnat, struct mnat_sess *mnat_sess,
		const struct menc *menc, struct menc_sess *menc_sess,
		uint32_t ptime, const struct list *aucodecl,
		audio_event_h *eventh, audio_err_h *errh, void *arg)
{
	struct audio *a;
	struct autx *tx;
	struct aurx *rx;
	struct le *le;
	int err;

	if (!ap || !cfg)
		return EINVAL;

	a = mem_zalloc(sizeof(*a), audio_destructor);
	if (!a)
		return ENOMEM;

	a->call = call;
	a->cfg = cfg->audio;
	tx = &a->tx;
	rx = &a->rx;

	err = stream_alloc(&a->strm, &cfg->avt, call, sdp_sess,
			   "audio", label,
			   mnat, mnat_sess, menc, menc_sess,
			   call_localuri(call),
			   stream_recv_handler, NULL, a);
	if (err)
		goto out;

	if (cfg->avt.rtp_bw.max) {
		stream_set_bw(a->strm, AUDIO_BANDWIDTH);
	}

	err = sdp_media_set_lattr(stream_sdpmedia(a->strm), true,
				  "ptime", "%u", ptime);
	if (err)
		goto out;

	/* Audio codecs */
	for (le = list_head(aucodecl); le; le = le->next) {
		err = add_audio_codec(a, stream_sdpmedia(a->strm), le->data);
		if (err)
			goto out;
	}

	tx->mb = mbuf_alloc(STREAM_PRESZ + 4096);
	if (!tx->mb) {
		err = ENOMEM;
		goto out;
	}

	err = telev_alloc(&a->telev, TELEV_PTIME);
	if (err)
		goto out;

	err = add_telev_codec(a);
	if (err)
		goto out;

	tx->ptime  = ptime;
	tx->ts     = rand_u16();
	tx->marker = true;

	rx->pt     = -1;
	rx->ptime  = ptime;

	a->eventh  = eventh;
	a->errh    = errh;
	a->arg     = arg;

 out:
	if (err)
		mem_deref(a);
	else
		*ap = a;

	return err;
}

/**
 * Start the audio playback and recording
 *
 * @param a Audio object
 *
 * @return 0 if success, otherwise errorcode
 */

int audio_start(struct audio *a)
{
	int err = 0;

	if (!a)
		return EINVAL;

	/* Here start to send and receive audio pkts */
	

	if (a->tx.ac && a->rx.ac) {
		a->started = true;
	}

	return err;
}


/**
 * Stop the audio playback and recording
 *
 * @param a Audio object
 */
void audio_stop(struct audio *a)
{
	if (!a)
		return;

	stop_tx(&a->tx, a);
	stop_rx(&a->rx);
}

extern int ep_update_audio_params(void *gateway, const char* peer, const char *cdcname, int srate, int ch, const char *fmtp);

int audio_encoder_set(struct audio *a, const struct aucodec *ac,
		      int pt_tx, const char *params)
{
	struct autx *tx;
	int err = 0;
	bool reset;

	if (!a || !ac)
		return EINVAL;

	tx = &a->tx;

	reset = !aucodec_equal(ac, tx->ac);

	if (ac != tx->ac) {
		info("audio: Set audio encoder: %s %uHz %dch\n",
		     ac->name, get_srate(ac), ac->ch);

		tx->is_g722 = (0 == str_casecmp(ac->name, "G722"));
		tx->ac = ac;
	}

	/* Should anounce the gateway about the encode params */
	if (reset && call_get_ua(a->call)) {
		ep_update_audio_params((call_get_ua(a->call))->owner->ep, call_peeruri(a->call), ac->name, get_srate(ac), ac->ch, params);
		settledown_audio_codec(stream_sdpmedia(a->strm), ac->name, get_srate(ac), ac->ch);
	}

	stream_set_srate(a->strm, get_srate(ac), get_srate(ac));
	stream_update_encoder(a->strm, pt_tx);

	
	err |= audio_start(a);

	return err;
}


int audio_decoder_set(struct audio *a, const struct aucodec *ac,
		      int pt_rx, const char *params)
{
	struct aurx *rx;
	bool reset;
	int err = 0;
	(void)params;

	if (!a || !ac)
		return EINVAL;

	rx = &a->rx;

	reset = !aucodec_equal(ac, rx->ac);

	if (ac != rx->ac) {

		info("audio: Set audio decoder: %s %uHz %dch\n",
		     ac->name, get_srate(ac), ac->ch);

		rx->pt = pt_rx;
		rx->ac = ac;
	}

	stream_set_srate(a->strm, get_srate(ac), get_srate(ac));

	if (reset) {
		err |= audio_start(a);
	}

	return err;
}


struct stream *audio_strm(const struct audio *a)
{
	return a ? a->strm : NULL;
}


int audio_send_digit(struct audio *a, char key)
{
	int err = 0;

	if (!a)
		return EINVAL;

	if (key > 0) {
		info("audio: send DTMF digit: '%c'\n", key);
		err = telev_send(a->telev, telev_digit2code(key), false);
	}
	else if (a->tx.cur_key) {
		/* Key release */
		info("audio: send DTMF digit end: '%c'\n", a->tx.cur_key);
		err = telev_send(a->telev,
				 telev_digit2code(a->tx.cur_key), true);
	}

	a->tx.cur_key = key;

	return err;
}


void audio_sdp_attr_decode(struct audio *a)
{
	const char *attr;

	if (!a)
		return;

	/* This is probably only meaningful for audio data, but
	   may be used with other media types if it makes sense. */
	attr = sdp_media_rattr(stream_sdpmedia(a->strm), "ptime");
	if (attr) {
		struct autx *tx = &a->tx;
		uint32_t ptime_tx = atoi(attr);

		if (ptime_tx && ptime_tx != a->tx.ptime) {

			info("audio: peer changed ptime_tx %ums -> %ums\n",
			     a->tx.ptime, ptime_tx);

			tx->ptime = ptime_tx;

			if (tx->ac) {
				tx->psize = 2 * get_framesize(tx->ac,
							      ptime_tx);
			}
		}
	}
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

void audio_send(struct audio *a, uint8_t *data, size_t len)
{
	struct rtp_header hdr;
	struct autx *tx = NULL;
	int err, pl_pos, pl_len;

    if (!a) {
		/*warning("audio_send: a == NULL.\n");*/
		goto out;
	}

	if (len < RTP_HEADER_SIZE) {
		warning("audio_send: len < 12\n");
		goto out;
	}

    err = pre_decode_rtpheader(&hdr, data, len);
    if (err) {
		warning("audio_send: rtp_hdr_decode failed. err=%d.\n", err);
		goto out;
	}

	tx = &a->tx;
    tx->mb->pos = tx->mb->end = STREAM_PRESZ;
    pl_pos = hdr.ext ? RTP_HEADER_SIZE + 4 * hdr.cc + 4 + hdr.x.len : RTP_HEADER_SIZE + 4 * hdr.cc;
    pl_len = len - pl_pos;

    err = mbuf_write_mem(tx->mb, data + pl_pos , pl_len);
	if (err) {
		warning("audio_send: mbuf_write_mem failed.\n");
		goto out;
	}
	
	tx->mb->pos = STREAM_PRESZ;
	tx->mb->end = STREAM_PRESZ + pl_len;
	err = stream_send(a->strm, hdr.m, -1, hdr.ts, tx->mb);
	if (err){
		warning("audio_send: stream_send failed.\n");
		goto out;
	}

 out:
    if (tx) {
      tx->marker = false;
    }
}
