/**
 * @file epoll.c  epoll specific routines
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <unistd.h>
#include <sys/epoll.h>
#include <re_types.h>
#include <re_mbuf.h>
#include <re_sys.h>
#include "main.h"


#define DEBUG_MODULE "epoll"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * Check for working epoll() kernel support
 *
 * @return true if support, false if not
 */
bool epoll_check(void)
{
	uint32_t osrel;
	int err, epfd;

	err = sys_rel_get(&osrel, NULL, NULL, NULL);
	if (err)
		return false;

	if (osrel < 0x020542) {
		DEBUG_INFO("epoll not supported in osrel=0x%08x\n", osrel);
		return false;
	}

#ifdef OPENWRT
	/* epoll is working again with 2.6.25.7 */
	if (osrel < 0x020619) {
		DEBUG_NOTICE("epoll is broken in osrel=0x%08x\n", osrel);
		return false;
	}
#endif

	epfd = epoll_create(64);
	if (-1 == epfd) {
		DEBUG_NOTICE("epoll_create: %m\n", errno);
		return false;
	}

	(void)close(epfd);

	return true;
}
