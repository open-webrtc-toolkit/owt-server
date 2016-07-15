/**
 * @file re_net.h  Interface to Networking module.
 *
 * Copyright (C) 2010 Creytiv.com
 */
#ifdef CYGWIN
#include <ws2tcpip.h>
#include <winsock2.h>
#elif defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#ifndef _BSD_SOCKLEN_T_
#define _BSD_SOCKLEN_T_ int  /**< Defines the BSD socket length type */
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


#ifndef HAVE_GAI_STRERROR
/** stub */
#ifndef gai_strerror
#define gai_strerror(err) "?"
#endif
#endif

/** Length of IPv4 address string */
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

/** Length of IPv6 address string */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/** Length of IPv4/v6 address string */
#ifdef HAVE_INET6
#define NET_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#define NET_ADDRSTRLEN INET_ADDRSTRLEN
#endif

/* forward declarations */
struct sa;


/* Net generic */
int  net_hostaddr(int af, struct sa *ip);
int  net_default_source_addr_get(int af, struct sa *ip);
int  net_default_gateway_get(int af, struct sa *gw);


/* Net sockets */
int  net_sock_init(void);
void net_sock_close(void);


/* Net socket options */
int net_sockopt_blocking_set(int fd, bool blocking);
int net_sockopt_reuse_set(int fd, bool reuse);


/* Net interface (if.c) */

/**
 * Defines the interface address handler - called once per interface
 *
 * @param ifname Name of the interface
 * @param sa     IP address of the interface
 * @param arg    Handler argument
 *
 * @return true to stop traversing, false to continue
 */
typedef bool (net_ifaddr_h)(const char *ifname, const struct sa *sa,
			    void *arg);

int net_if_getname(char *ifname, size_t sz, int af, const struct sa *ip);
int net_if_getaddr(const char *ifname, int af, struct sa *ip);
int net_if_getaddr4(const char *ifname, int af, struct sa *ip);
int net_if_list(net_ifaddr_h *ifh, void *arg);
int net_if_apply(net_ifaddr_h *ifh, void *arg);
int net_if_debug(struct re_printf *pf, void *unused);
int net_if_getlinklocal(const char *ifname, int af, struct sa *ip);


/* Net interface (ifaddrs.c) */
int net_getifaddrs(net_ifaddr_h *ifh, void *arg);


/* Net route */

/**
 * Defines the routing table handler - called once per route entry
 *
 * @param ifname Interface name
 * @param dst    Destination IP address/network
 * @param dstlen Prefix length of destination
 * @param gw     Gateway IP address
 * @param arg    Handler argument
 *
 * @return true to stop traversing, false to continue
 */
typedef bool (net_rt_h)(const char *ifname, const struct sa *dst,
			int dstlen, const struct sa *gw, void *arg);

int net_rt_list(net_rt_h *rth, void *arg);
int net_rt_default_get(int af, char *ifname, size_t size);
int net_rt_debug(struct re_printf *pf, void *unused);


/* Network connection */

/**
 * Defines the network connection handler
 *
 * @param err  Error code
 * @param id   Associated ID
 */
typedef void (net_conn_h)(int err, uint32_t id);

int  net_conn_start(net_conn_h *ch, uint32_t id, bool prompt);
void net_conn_stop(void);


/* Net strings */
const char *net_proto2name(int proto);
const char *net_af2name(int af);


/* todo: this does not really belong here.. */
#ifdef __SYMBIAN32__
int kerr2errno(int kerr);
#endif
