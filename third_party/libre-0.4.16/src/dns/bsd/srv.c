/**
 * @file bsd/srv.c  Get DNS Server IP using libresolv
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _BSD_SOURCE 1
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_dns.h>
#include "../dns.h"


int get_resolv_dns(char *domain, size_t dsize, struct sa *nsv, uint32_t *n)
{
	struct __res_state state;
	uint32_t i;
	int ret, err;

	memset(&state, 0, sizeof(state));
	ret = res_ninit(&state);
	if (0 != ret)
		return ENOENT;

	if (state.dnsrch[0])
		str_ncpy(domain, state.dnsrch[0], dsize);
	else if (state.defdname)
		str_ncpy(domain, state.defdname, dsize);

	if (!state.nscount) {
		err = ENOENT;
		goto out;
	}

	err = 0;
	for (i=0; i<min(*n, (uint32_t)state.nscount) && !err; i++) {
		struct sockaddr_in *addr = &state.nsaddr_list[i];
		err |= sa_set_sa(&nsv[i], (struct sockaddr *)addr);
	}
	if (err)
		goto out;

	*n = i;

 out:
	res_nclose(&state);

	return err;
}
