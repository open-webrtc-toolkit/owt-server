/**
 * @file ifaddrs.c  Network interface code using getifaddrs().
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <unistd.h>
#include <sys/socket.h>
#define __USE_MISC 1   /**< Use MISC code */
#include <net/if.h>
#include <ifaddrs.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_sa.h>
#include <re_net.h>


#define DEBUG_MODULE "ifaddrs"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * Get a list of all network interfaces including name and IP address.
 * Both IPv4 and IPv6 are supported.
 *
 * @param ifh Interface handler, called once per network interface.
 * @param arg Handler argument.
 *
 * @return 0 if success, otherwise errorcode.
 */
int net_getifaddrs(net_ifaddr_h *ifh, void *arg)
{
	struct ifaddrs *ifa, *ifp;
	int err;

	if (!ifh)
		return EINVAL;

	if (0 != getifaddrs(&ifa)) {
		err = errno;
		DEBUG_WARNING("getifaddrs: %m\n", err);
		return err;
	}

	for (ifp = ifa; ifa; ifa = ifa->ifa_next) {
		struct sa sa;

		DEBUG_INFO("ifaddr: %10s flags=%08x\n", ifa->ifa_name,
			   ifa->ifa_flags);

		if (ifa->ifa_flags & IFF_UP) {
			err = sa_set_sa(&sa, ifa->ifa_addr);
			if (err)
				continue;

			if (ifh(ifa->ifa_name, &sa, arg))
				break;
		}
	}

	freeifaddrs(ifp);

	return 0;
}
