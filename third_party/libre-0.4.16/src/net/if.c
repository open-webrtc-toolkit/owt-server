/**
 * @file net/if.c  Network interface code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>


#define DEBUG_MODULE "netif"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/** Interface address entry */
struct ifentry {
	int af;        /**< Address family */
	char *ifname;  /**< Interface name */
	struct sa *ip; /**< IP address     */
	size_t sz;     /**< Size of buffer */
	bool found;    /**< Found flag     */
};


static bool if_getname_handler(const char *ifname, const struct sa *sa,
			       void *arg)
{
	struct ifentry *ife = arg;

	if (ife->af != sa_af(sa))
		return false;

	if (0 == sa_cmp(sa, ife->ip, SA_ADDR)) {
		str_ncpy(ife->ifname, ifname, ife->sz);
		ife->found = true;
		return true;
	}

	return false;
}


/**
 * Get the name of the interface for a given IP address
 *
 * @param ifname Buffer for returned network interface name
 * @param sz     Size of buffer
 * @param af     Address Family
 * @param ip     Given IP address
 *
 * @return 0 if success, otherwise errorcode
 */
int net_if_getname(char *ifname, size_t sz, int af, const struct sa *ip)
{
	struct ifentry ife;
	int err;

	if (!ifname || !sz || !ip)
		return EINVAL;

	ife.af     = af;
	ife.ifname = ifname;
	ife.ip     = (struct sa *)ip;
	ife.sz     = sz;
	ife.found  = false;

	err = net_if_list(if_getname_handler, &ife);

	return ife.found ? err : ENODEV;
}


static bool if_getaddr_handler(const char *ifname,
			       const struct sa *sa, void *arg)
{
	struct ifentry *ife = arg;

	/* Match name of interface? */
	if (str_isset(ife->ifname) && 0 != str_casecmp(ife->ifname, ifname))
		return false;

	if (!sa_isset(sa, SA_ADDR))
		return false;

#if 1
	/* skip loopback and link-local IP */
	if (sa_is_loopback(sa) || sa_is_linklocal(sa))
		return false;
#endif

	/* Match address family */
	if (ife->af != sa_af(sa))
		return false;

	/* Match - copy address */
	sa_cpy(ife->ip, sa);
	ife->found = true;

	return ife->found;
}


/**
 * Get IP address for a given network interface
 *
 * @param ifname  Network interface name (optional)
 * @param af      Address Family
 * @param ip      Returned IP address
 *
 * @return 0 if success, otherwise errorcode
 *
 * @deprecated Works for IPv4 only
 */
int net_if_getaddr(const char *ifname, int af, struct sa *ip)
{
	struct ifentry ife;
	int err;

	if (!ip)
		return EINVAL;

	ife.af     = af;
	ife.ifname = (char *)ifname;
	ife.ip     = ip;
	ife.sz     = 0;
	ife.found  = false;

#ifdef HAVE_GETIFADDRS
	err = net_getifaddrs(if_getaddr_handler, &ife);
#else
	err = net_if_list(if_getaddr_handler, &ife);
#endif

	return ife.found ? err : ENODEV;
}


#ifndef __SYMBIAN32__
static bool if_debug_handler(const char *ifname, const struct sa *sa,
			     void *arg)
{
	struct re_printf *pf = arg;

	(void)re_hprintf(pf, " %10s:  %j\n", ifname, sa);

	return false;
}


/**
 * Debug network interfaces
 *
 * @param pf     Print handler for debug output
 * @param unused Unused parameter
 *
 * @return 0 if success, otherwise errorcode
 */
int net_if_debug(struct re_printf *pf, void *unused)
{
	int err;

	(void)unused;

	err = re_hprintf(pf, "net interfaces:\n");

#ifdef HAVE_GETIFADDRS
	err |= net_getifaddrs(if_debug_handler, pf);
#else
	err |= net_if_list(if_debug_handler, pf);
#endif

	return err;
}
#endif


static bool linklocal_handler(const char *ifname, const struct sa *sa,
			      void *arg)
{
	void **argv = arg;
	int af = *(int *)argv[1];

	if (argv[0] && 0 != str_casecmp(argv[0], ifname))
		return false;

	if (af != AF_UNSPEC && af != sa_af(sa))
		return false;

	if (sa_is_linklocal(sa)) {
		*((struct sa *)argv[2]) = *sa;
		return true;
	}

	return false;
}


/**
 * Get the Link-local address for a specific network interface
 *
 * @param ifname Name of the interface
 * @param af     Address family
 * @param ip     Returned link-local address
 *
 * @return 0 if success, otherwise errorcode
 */
int net_if_getlinklocal(const char *ifname, int af, struct sa *ip)
{
	struct sa addr;
	void *argv[3];
	int err;

	if (!ip)
		return EINVAL;

	sa_init(&addr, sa_af(ip));

	argv[0] = (void *)ifname;
	argv[1] = &af;
	argv[2] = &addr;

	err = net_if_apply(linklocal_handler, argv);
	if (err)
		return err;

	if (!sa_isset(&addr, SA_ADDR))
		return ENOENT;

	*ip = addr;

	return 0;
}
