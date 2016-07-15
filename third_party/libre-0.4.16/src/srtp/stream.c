/**
 * @file srtp/stream.c  Secure Real-time Transport Protocol (SRTP) -- stream
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_srtp.h>
#include "srtp.h"


/** SRTP protocol values */
#ifndef SRTP_MAX_STREAMS
#define SRTP_MAX_STREAMS  (8)  /**< Maximum number of SRTP streams */
#endif


static void stream_destructor(void *arg)
{
	struct srtp_stream *strm = arg;

	list_unlink(&strm->le);
}


static struct srtp_stream *stream_find(struct srtp *srtp, uint32_t ssrc)
{
	struct le *le;

	for (le = srtp->streaml.head; le; le = le->next) {

		struct srtp_stream *strm = le->data;

		if (strm->ssrc == ssrc)
			return strm;
	}

	return NULL;
}


static int stream_new(struct srtp_stream **strmp, struct srtp *srtp,
		      uint32_t ssrc)
{
	struct srtp_stream *strm;

	if (list_count(&srtp->streaml) >= SRTP_MAX_STREAMS)
		return ENOSR;

	strm = mem_zalloc(sizeof(*strm), stream_destructor);
	if (!strm)
		return ENOMEM;

	strm->ssrc = ssrc;
	srtp_replay_init(&strm->replay_rtp);
	srtp_replay_init(&strm->replay_rtcp);

	list_append(&srtp->streaml, &strm->le, strm);

	if (strmp)
		*strmp = strm;

	return 0;
}


int stream_get(struct srtp_stream **strmp, struct srtp *srtp, uint32_t ssrc)
{
	struct srtp_stream *strm;

	if (!strmp || !srtp)
		return EINVAL;

	strm = stream_find(srtp, ssrc);
	if (strm) {
		*strmp = strm;
		return 0;
	}

	return stream_new(strmp, srtp, ssrc);
}


int stream_get_seq(struct srtp_stream **strmp, struct srtp *srtp,
		   uint32_t ssrc, uint16_t seq)
{
	struct srtp_stream *strm;
	int err;

	if (!strmp || !srtp)
		return EINVAL;

	err = stream_get(&strm, srtp, ssrc);
	if (err)
		return err;

	/* Set the initial sequence number once only */
	if (!strm->s_l_set) {
		strm->s_l = seq;
		strm->s_l_set = true;
	}

	*strmp = strm;

	return 0;
}
