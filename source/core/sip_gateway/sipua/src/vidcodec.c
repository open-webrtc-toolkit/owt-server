/**
 * @file vidcodec.c Video Codec
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re.h>
#include <baresip.h>
#include "vp8.h"


static struct list vidcodecl;


/**
 * Register a Video Codec
 *
 * @param vc Video Codec
 */
void vidcodec_register(struct vidcodec *vc)
{
	if (!vc)
		return;

	list_append(&vidcodecl, &vc->le, vc);

	info("vidcodec: %s\n", vc->name);
}


/**
 * Unregister a Video Codec
 *
 * @param vc Video Codec
 */
void vidcodec_unregister(struct vidcodec *vc)
{
	if (!vc)
		return;

	list_unlink(&vc->le);
}


/**
 * Find a Video Codec by name
 *
 * @param name    Name of the Video Codec to find
 * @param variant Codec Variant
 *
 * @return Matching Video Codec if found, otherwise NULL
 */
const struct vidcodec *vidcodec_find(const char *name, const char *variant)
{
	struct le *le;

	for (le=vidcodecl.head; le; le=le->next) {

		struct vidcodec *vc = le->data;

		if (name && 0 != str_casecmp(name, vc->name))
			continue;

		if (variant && 0 != str_casecmp(variant, vc->variant))
			continue;

		return vc;
	}

	return NULL;
}


/**
 * Find a Video Encoder by name
 *
 * @param name    Name of the Video Encoder to find
 *
 * @return Matching Video Encoder if found, otherwise NULL
 */
const struct vidcodec *vidcodec_find_encoder(const char *name)
{
	struct le *le;

	for (le=vidcodecl.head; le; le=le->next) {

		struct vidcodec *vc = le->data;

		if (name && 0 != str_casecmp(name, vc->name))
			continue;

		if (vc->ench)
			return vc;
	}

	return NULL;
}


/**
 * Find a Video Decoder by name
 *
 * @param name    Name of the Video Decoder to find
 *
 * @return Matching Video Decoder if found, otherwise NULL
 */
const struct vidcodec *vidcodec_find_decoder(const char *name)
{
	struct le *le;

	for (le=vidcodecl.head; le; le=le->next) {

		struct vidcodec *vc = le->data;

		if (name && 0 != str_casecmp(name, vc->name))
			continue;

		if (vc->dech)
			return vc;
	}

	return NULL;
}


/**
 * Get the list of Video Codecs
 *
 * @return List of Video Codecs
 */
struct list *vidcodec_list(void)
{
	return &vidcodecl;
}





/*********  supported video codecs **********/


/* VP8 */
static int vp8_fmtp_enc(struct mbuf *mb, const struct sdp_format *fmt,
		 bool offer, void *arg)
{
	const struct vp8_vidcodec *vp8 = arg;
	(void)offer;

	if (!mb || !fmt || !vp8 || !vp8->max_fs)
		return 0;

	return mbuf_printf(mb, "a=fmtp:%s max-fs=%u\r\n",
			   fmt->id, vp8->max_fs);
}


static struct vp8_vidcodec vpx = {
	.vc = {
		.name      = "VP8",
		.encupdh   = NULL,
		.ench      = NULL,
		.decupdh   = NULL,
		.dech      = NULL,
		.fmtp_ench = vp8_fmtp_enc,
	},
	.max_fs = 3600
};


/* H264 */
#if 0
static int h264_fmtp_enc_mode0(struct mbuf *mb, const struct sdp_format *fmt,
			 bool offer, void *arg)
{
	struct vidcodec *vc = arg;
	const uint8_t profile_idc = 0x42; /* baseline profile */
	const uint8_t profile_iop = 0x80;
	(void)offer;

	if (!mb || !fmt || !vc)
		return 0;

	return mbuf_printf(mb, "a=fmtp:%s"
			   " packetization-mode=0"
			   ";profile-level-id=%02x%02x%02x"
			   ";in-band-parameter-sets=1"
			   "\r\n",
			   fmt->id, profile_idc, profile_iop, 0x1F);
}

static int h264_fmtp_enc_mode1(struct mbuf *mb, const struct sdp_format *fmt,
			 bool offer, void *arg)
{
	struct vidcodec *vc = arg;
	const uint8_t profile_idc = 0x42; /* baseline profile */
	const uint8_t profile_iop = 0x80;
	(void)offer;

	if (!mb || !fmt || !vc)
		return 0;

	return mbuf_printf(mb, "a=fmtp:%s"
			   " packetization-mode=1"
			   ";profile-level-id=%02x%02x%02x"
			   ";in-band-parameter-sets=1"
			   "\r\n",
			   fmt->id, profile_idc, profile_iop, 0x1F);
}
#endif
static uint32_t packetization_mode(const char *fmtp)
{
	struct pl pl, mode;

	if (!fmtp)
		return 0;

	pl_set_str(&pl, fmtp);

	if (fmt_param_get(&pl, "packetization-mode", &mode))
		return pl_u32(&mode);

	return 0;
}


static bool h264_fmtp_cmp(const char *fmtp1, const char *fmtp2, void *data)
{
	(void)data;

	return packetization_mode(fmtp1) == packetization_mode(fmtp2);
}

static struct vidcodec h264_mode0 = {
	.name      = "H264",
	.variant   = "packetization-mode=0",
	.encupdh   = NULL,
	.ench      = NULL,
	.decupdh   = NULL,
	.dech      = NULL,
	/*.fmtp_ench = h264_fmtp_enc_mode0,*/
	.fmtp_ench = NULL,
	.fmtp = "packetization-mode=0;profile-level-id=42801F;in-band-parameter-sets=1",
	.fmtp_cmph = h264_fmtp_cmp,
};

static struct vidcodec h264_mode1 = {
	.name      = "H264",
	.variant   = "packetization-mode=1",
	.encupdh   = NULL,
	.ench      = NULL,
	.decupdh   = NULL,
	.dech      = NULL,
	/*.fmtp_ench = h264_fmtp_enc_mode1,*/
	.fmtp_ench = NULL,
	.fmtp = "packetization-mode=1;profile-level-id=42801F;in-band-parameter-sets=1",
	.fmtp_cmph = h264_fmtp_cmp,
};

void register_video_codecs(void)
{
	vidcodec_register((struct vidcodec *)&vpx);
	vidcodec_register(&h264_mode0);
	vidcodec_register(&h264_mode1);
}
