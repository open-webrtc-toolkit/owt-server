/**
 * @file net/rt.c  Generic routing table code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>


struct net_rt {
	int af;
	char *ifname;
	size_t size;
	int prefix;
};


static bool rt_debug_handler(const char *ifname, const struct sa *dst,
			     int dstlen, const struct sa *gw, void *arg)
{
	char addr[64];
	struct re_printf *pf = arg;
	int err = 0;

	(void)re_snprintf(addr, sizeof(addr), "%j/%d", dst, dstlen);

	err |= re_hprintf(pf, " %-44s", addr);
	err |= re_hprintf(pf, "%-40j", gw);
	err |= re_hprintf(pf, " %-15s ", ifname);

#ifdef HAVE_INET6
	if (AF_INET6 == sa_af(dst)) {
		const struct sockaddr_in6 *sin6 = &dst->u.in6;
		const struct in6_addr *in6 = &sin6->sin6_addr;

		if (IN6_IS_ADDR_MULTICAST(in6))
			err |= re_hprintf(pf, " MULTICAST");
		if (IN6_IS_ADDR_LINKLOCAL(in6))
			err |= re_hprintf(pf, " LINKLOCAL");
		if (IN6_IS_ADDR_SITELOCAL(in6))
			err |= re_hprintf(pf, " SITELOCAL");
	}
#endif

	err |= re_hprintf(pf, "\n");

	return 0 != err;
}


/**
 * Dump the routing table
 *
 * @param pf     Print function for output
 * @param unused Unused parameter
 *
 * @return 0 if success, otherwise errorcode
 */
int net_rt_debug(struct re_printf *pf, void *unused)
{
	int err = 0;

	(void)unused;

	err |= re_hprintf(pf, "net routes:\n");

	err |= re_hprintf(pf, " Destination                                 "
			  "Next Hop"
			  "                                 Iface           "
			  "Type\n");

	err |= net_rt_list(rt_debug_handler, pf);

	return err;
}


static bool rt_default_get_handler(const char *_ifname, const struct sa *dst,
				   int dstlen, const struct sa *gw, void *arg)
{
	struct net_rt *rt = arg;

	(void)dstlen;
	(void)gw;

	if (sa_af(dst) != rt->af)
		return false;

	switch (rt->af) {

	case AF_INET:
		if (0 == sa_in(dst)) {
			str_ncpy(rt->ifname, _ifname, rt->size);
			return true;
		}
		break;

#ifdef HAVE_INET6
	case AF_INET6:
		if (IN6_IS_ADDR_MULTICAST(&dst->u.in6.sin6_addr))
			return false;
		if (IN6_IS_ADDR_LINKLOCAL(&dst->u.in6.sin6_addr))
			return false;

		if (dstlen < rt->prefix) {
			rt->prefix = dstlen;
			str_ncpy(rt->ifname, _ifname, rt->size);
			return false;
		}
		break;
#endif
	}

	return false;
}


/**
 * Get the interface name of the default route
 *
 * @param af     Address family
 * @param ifname Buffer for returned interface name
 * @param size   Size of buffer
 *
 * @return 0 if success, otherwise errorcode
 */
int net_rt_default_get(int af, char *ifname, size_t size)
{
	struct net_rt rt;
	int err;

	rt.af     = af;
	rt.ifname = ifname;
	rt.size   = size;
	rt.prefix = 256;

	err = net_rt_list(rt_default_get_handler, &rt);
	if (err)
		return err;

	return '\0' != ifname[0] ? 0 : EINVAL;
}


#ifndef HAVE_ROUTE_LIST
/* We must provide a stub */
int net_rt_list(net_rt_h *rth, void *arg)
{
	(void)rth;
	(void)arg;
	return ENOSYS;
}
#endif
