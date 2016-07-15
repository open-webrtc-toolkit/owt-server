/**
 * @file posix/pif.c  POSIX network interface code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#define __USE_POSIX 1  /**< Use POSIX code */
#define __USE_XOPEN2K 1/**< Use POSIX.1:2001 code */
#include <netdb.h>
#define __USE_MISC 1   /**< Use MISC code */
#include <net/if.h>
#include <arpa/inet.h>
/*#include <net/if_arp.h>*/
#ifdef __sun
#include <sys/sockio.h>
#endif
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>


#define DEBUG_MODULE "posixif"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * Get IP address for a given network interface
 *
 * @param ifname  Network interface name
 * @param af      Address Family
 * @param ip      Returned IP address
 *
 * @return 0 if success, otherwise errorcode
 *
 * @deprecated Works for IPv4 only
 */
int net_if_getaddr4(const char *ifname, int af, struct sa *ip)
{
	struct addrinfo hints, *res, *r;
	int error, err;

	if (AF_INET != af)
		return EAFNOSUPPORT;

	memset(&hints, 0, sizeof(hints));
	/* set-up hints structure */
	hints.ai_family   = PF_UNSPEC;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(NULL, "0", &hints, &res);
	if (error) {
		DEBUG_WARNING("get_ifaddr: getaddrinfo(): %s\n",
			      gai_strerror(error));
		return EADDRNOTAVAIL;
	}

	err = ENOENT;
	for (r = res; r; r = r->ai_next) {
		struct ifreq ifrr;
		int fd = -1;

		fd = socket(r->ai_family, SOCK_DGRAM, 0);
		if (fd < 0) {
			continue;
		}

		ifrr.ifr_addr.sa_family = r->ai_family;
		str_ncpy(ifrr.ifr_name, ifname, sizeof(ifrr.ifr_name));

		if (ioctl(fd, SIOCGIFADDR, &ifrr) < 0) {
			err = errno;
			goto next;
		}

		err = sa_set_sa(ip, &ifrr.ifr_ifru.ifru_addr);

	next:
		(void)close(fd);
	}

	freeaddrinfo(res);
	return err;
}


/**
 * Enumerate all network interfaces
 *
 * @param ifh Interface handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode
 *
 * @deprecated Works for IPv4 only
 */
int net_if_list(net_ifaddr_h *ifh, void *arg)
{
	struct ifreq ifrv[32], *ifr;
	struct ifconf ifc;
	int sockfd = -1;
	int err = 0;

	if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))) {
		err = errno;
		DEBUG_WARNING("interface list: socket(): (%m)\n", err);
		goto out;
	}

	ifc.ifc_len = sizeof(ifrv);
	ifc.ifc_req = ifrv;

	if (0 != ioctl(sockfd, SIOCGIFCONF, &ifc)) {
		err = errno;
		DEBUG_WARNING("interface list: ioctl SIOCFIFCONF: %m\n", err);
		goto out;
	}

	for (ifr = ifc.ifc_req;
	     (char *)ifr < ((char *)ifc.ifc_buf + ifc.ifc_len);
	     ++ifr) {
		struct ifreq ifrr;
		struct sa sa;

		if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data)
			continue;  /* duplicate, skip it */

		if (ioctl(sockfd, SIOCGIFFLAGS, ifr))
			continue;  /* failed to get flags, skip it */

#if 0
		if (ifr->ifr_flags & IFF_LOOPBACK)
			continue;
#endif

		if (!(ifr->ifr_flags & IFF_UP))
			continue;

		ifrr.ifr_addr.sa_family = AF_INET;
		str_ncpy(ifrr.ifr_name, ifr->ifr_name, sizeof(ifrr.ifr_name));

		if (ioctl(sockfd, SIOCGIFADDR, &ifrr) < 0) {
			err = errno;
			continue;
		}

		err = sa_set_sa(&sa, &ifrr.ifr_ifru.ifru_addr);
		if (err) {
			DEBUG_WARNING("if_list: sa_set_sa %m\n", err);
			break;
		}

		if (ifh && ifh(ifr->ifr_name, &sa, arg))
			break;
	}

 out:
	if (sockfd >= 0)
		(void)close(sockfd);

	return err;
}
