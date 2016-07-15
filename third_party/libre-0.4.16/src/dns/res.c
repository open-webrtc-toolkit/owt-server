/**
 * @file res.c  Get DNS Server IP using libresolv
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_dns.h>
#include "dns.h"


/**
 * Generic way of fetching Nameserver IP-addresses, using libresolv
 *
 * @param domain Returned domain name
 * @param dsize  Size of domain name buffer
 * @param nsv    Returned nameservers
 * @param n      Nameservers capacity, actual on return
 *
 * @note we could use res_getservers() but it is not available on Linux
 * @note only IPv4 is supported
 *
 * @return 0 if success, otherwise errorcode
 */
int get_resolv_dns(char *domain, size_t dsize, struct sa *nsv, uint32_t *n)
{
	uint32_t i;
	int ret, err;

	ret = res_init();
	if (0 != ret)
		return ENOENT;

	if (_res.dnsrch[0])
		str_ncpy(domain, _res.dnsrch[0], dsize);
	else if ((char *)_res.defdname)
		str_ncpy(domain, _res.defdname, dsize);

	if (!_res.nscount) {
		err = ENOENT;
		goto out;
	}

	err = 0;
	for (i=0; i<min(*n, (uint32_t)_res.nscount) && !err; i++) {
		struct sockaddr_in *addr = &_res.nsaddr_list[i];
		err |= sa_set_sa(&nsv[i], (struct sockaddr *)addr);
	}
	if (err)
		goto out;

	*n = i;

 out:
	res_close();

	return err;
}
