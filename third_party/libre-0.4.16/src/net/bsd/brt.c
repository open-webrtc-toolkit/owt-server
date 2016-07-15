/**
 * @file bsd/brt.c  BSD routing table code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_sa.h>
#include <re_net.h>
#include <sys/sysctl.h>
#include <net/route.h>
#include <net/if.h>


/*
 * See https://github.com/boundary/libdnet/blob/master/src/route-bsd.c
 */

#ifdef __APPLE__
#define RT_MSGHDR_ALIGNMENT sizeof(uint32_t)
#else
#define RT_MSGHDR_ALIGNMENT sizeof(unsigned long)
#endif

#define ROUNDUP(a) \
	((a) > 0						\
	 ? (1 + (((size_t)(a) - 1) | (RT_MSGHDR_ALIGNMENT - 1))) \
	 : RT_MSGHDR_ALIGNMENT)


int net_rt_list(net_rt_h *rth, void *arg)
{
	/* net.route.0.inet.flags.gateway */
	int mib[] = {CTL_NET, PF_ROUTE, 0, AF_UNSPEC,
	             NET_RT_FLAGS, RTF_GATEWAY};
	char ifname[IFNAMSIZ], *buf, *p;
	struct rt_msghdr *rt;
	struct sockaddr *sa, *sa_tab[RTAX_MAX];
	struct sa dst, gw;
	size_t l;
	int i, err = 0;

	if (sysctl(mib, sizeof(mib)/sizeof(int), 0, &l, 0, 0) < 0)
		return errno;
	if (!l)
		return ENOENT;

	buf = mem_alloc(l, NULL);
	if (!buf)
		return ENOMEM;

	if (sysctl(mib, sizeof(mib)/sizeof(int), buf, &l, 0, 0) < 0) {
		err = errno;
		goto out;
	}

	for (p = buf; p<buf+l; p += rt->rtm_msglen) {
		rt = (void *)p;  /* buffer is aligned */
		sa = (struct sockaddr *)(rt + 1);

		if (rt->rtm_type != RTM_GET)
			continue;

		if (!(rt->rtm_flags & RTF_UP))
			continue;

		for (i=0; i<RTAX_MAX; i++) {

			if (rt->rtm_addrs & (1 << i)) {
				sa_tab[i] = sa;
				sa = (struct sockaddr *)
					((char *)sa + ROUNDUP(sa->sa_len));
			}
			else {
				sa_tab[i] = NULL;
			}
		}

		if ((rt->rtm_addrs & RTA_DST) == RTA_DST) {
			err = sa_set_sa(&dst, sa_tab[RTAX_DST]);
			if (err)
				continue;
		}
		if ((rt->rtm_addrs & RTA_GATEWAY) == RTA_GATEWAY) {
			err = sa_set_sa(&gw, sa_tab[RTAX_GATEWAY]);
			if (err)
				continue;
		}

		if_indextoname(rt->rtm_index, ifname);

		if (rth(ifname, &dst, 0, &gw, arg))
			break;
	}

 out:
	mem_deref(buf);

	return err;
}
