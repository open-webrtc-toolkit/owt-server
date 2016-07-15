/**
 * @file srtp/replay.c  SRTP replay protection
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_srtp.h>
#include "srtp.h"


enum {
	SRTP_WINDOW_SIZE = 64
};


void srtp_replay_init(struct replay *replay)
{
	if (!replay)
		return;

	replay->bitmap = 0;
	replay->lix    = 0;
}


/*
 * Returns false if packet disallowed, true if packet permitted
 */
bool srtp_replay_check(struct replay *replay, uint64_t ix)
{
	uint64_t diff;

	if (!replay)
		return false;

	if (ix > replay->lix) {
		diff = ix - replay->lix;

		if (diff < SRTP_WINDOW_SIZE) {   /* In window */
			replay->bitmap <<= diff;
			replay->bitmap |= 1;     /* set bit for this packet */
		}
		else
			replay->bitmap = 1;

		replay->lix = ix;
		return true;
	}

	diff = replay->lix - ix;
	if (diff >= SRTP_WINDOW_SIZE)
		return false;

	if (replay->bitmap & (1ULL << diff))
		return false; /* already seen */

	/* mark as seen */
	replay->bitmap |= (1ULL << diff);

	return true;
}
