/**
 * @file linux/rt.c  Routing table code for Linux. See rtnetlink(7)
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#include <string.h>
#include <unistd.h>
#define __USE_POSIX 1  /**< Use POSIX flag */
#include <netdb.h>
#define __USE_MISC 1
#include <net/if.h>
#undef __STRICT_ANSI__
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <re_types.h>
#include <re_mbuf.h>
#include <re_fmt.h>
#include <re_sa.h>
#include <re_net.h>


#define DEBUG_MODULE "linuxrt"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/* Override macros to avoid casting alignment warning */
#undef RTM_RTA
#define RTM_RTA(r) (void *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct rtmsg)))
#undef RTA_NEXT
#define RTA_NEXT(rta, len) ((len) -= RTA_ALIGN((rta)->rta_len), \
		(void *)(((char *)(rta)) + RTA_ALIGN((rta)->rta_len)))
#undef NLMSG_NEXT
#define NLMSG_NEXT(nlh,len)	 ((len) -= NLMSG_ALIGN((nlh)->nlmsg_len), \
		  (void*)(((char*)(nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))


enum {BUFSIZE = 8192};


/** Defines a network route */
struct net_rt {
	char ifname[IFNAMSIZ];  /**< Interface name                 */
	struct sa dst;          /**< Destination IP address/network */
	int dstlen;             /**< Prefix length of destination   */
	struct sa gw;           /**< Gateway IP address             */
};


static int read_sock(int fd, uint8_t *buf, size_t size, int seq, int pid)
{
	struct nlmsghdr *nlhdr;
	int n = 0, len = 0;

	do {
		/* Receive response from the kernel */
		if ((n = recv(fd, buf, size - len, 0)) < 0) {
			DEBUG_WARNING("SOCK READ: %m\n", errno);
			return -1;
		}
		nlhdr = (struct nlmsghdr *)(void *)buf;

		/* Check if the header is valid */
		if (0 == NLMSG_OK(nlhdr, (uint32_t)n) ||
		    NLMSG_ERROR == nlhdr->nlmsg_type) {
			DEBUG_WARNING("Error in received packet\n");
			return -1;
		}

		/* Check if the its the last message */
		if (NLMSG_DONE == nlhdr->nlmsg_type) {
			break;
		}
		else{
			/* Else move the pointer to buffer appropriately */
			buf += n;
			len += n;
		}

		/* Check if its a multi part message */
		if (0 == (nlhdr->nlmsg_flags & NLM_F_MULTI)) {
			/* return if its not */
			break;
		}
	} while (nlhdr->nlmsg_seq != (uint32_t)seq ||
		 nlhdr->nlmsg_pid != (uint32_t)pid);

	return len;
}


/* Parse one route */
static int rt_parse(const struct nlmsghdr *nlhdr, struct net_rt *rt)
{
	struct rtmsg *rtmsg;
	struct rtattr *rtattr;
	int len;

	rtmsg = (struct rtmsg *)NLMSG_DATA(nlhdr);

	/* If the route does not belong to main routing table then return. */
	if (RT_TABLE_MAIN != rtmsg->rtm_table)
		return EINVAL;

	sa_init(&rt->dst, rtmsg->rtm_family);
	rt->dstlen = rtmsg->rtm_dst_len;

	/* get the rtattr field */
	rtattr = (struct rtattr *)RTM_RTA(rtmsg);
	len = RTM_PAYLOAD(nlhdr);
	for (;RTA_OK(rtattr, len); rtattr = RTA_NEXT(rtattr, len)) {

		switch (rtattr->rta_type) {

		case RTA_OIF:
			if_indextoname(*(int *)RTA_DATA(rtattr), rt->ifname);
			break;

		case RTA_GATEWAY:
			switch (rtmsg->rtm_family) {

			case AF_INET:
				sa_init(&rt->gw, AF_INET);
				rt->gw.u.in.sin_addr.s_addr
					= *(uint32_t *)RTA_DATA(rtattr);
				break;

#ifdef HAVE_INET6
			case AF_INET6:
				sa_set_in6(&rt->gw, RTA_DATA(rtattr), 0);
				break;
#endif

			default:
				DEBUG_WARNING("RTA_DST: unknown family %d\n",
					      rtmsg->rtm_family);
				break;
			}
			break;

#if 0
		case RTA_PREFSRC:
			rt->srcaddr = *(uint32_t *)RTA_DATA(rtattr);
			break;
#endif

		case RTA_DST:
			switch (rtmsg->rtm_family) {

			case AF_INET:
				sa_init(&rt->dst, AF_INET);
				rt->dst.u.in.sin_addr.s_addr
					= *(uint32_t *)RTA_DATA(rtattr);
				break;

#ifdef HAVE_INET6
			case AF_INET6:
				sa_set_in6(&rt->dst, RTA_DATA(rtattr), 0);
				break;
#endif

			default:
				DEBUG_WARNING("RTA_DST: unknown family %d\n",
					      rtmsg->rtm_family);
				break;
			}
			break;
		}
	}

	return 0;
}


/**
 * List all entries in the routing table
 *
 * @param rth Route entry handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int net_rt_list(net_rt_h *rth, void *arg)
{
	union {
		uint8_t buf[BUFSIZE];
		struct nlmsghdr msg[1];
	} u;
	struct nlmsghdr *nlmsg;
	int sock, len, seq = 0, err = 0;

	if (!rth)
		return EINVAL;

	/* Create Socket */
	if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
		DEBUG_WARNING("list: socket(): (%m)\n", errno);
		return errno;
	}

	/* Initialize the buffer */
	memset(u.buf, 0, sizeof(u.buf));

	/* point the header and the msg structure pointers into the buffer */
	nlmsg = u.msg;

	/* Fill in the nlmsg header*/
	nlmsg->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlmsg->nlmsg_type  = RTM_GETROUTE;
	nlmsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	nlmsg->nlmsg_seq   = seq++;
	nlmsg->nlmsg_pid   = getpid();

	/* Send the request */
	if (send(sock, nlmsg, nlmsg->nlmsg_len, 0) < 0) {
		err = errno;
		DEBUG_WARNING("list: write to socket failed (%m)\n", err);
		goto out;
	}

	/* Read the response */
	if ((len = read_sock(sock, u.buf, sizeof(u.buf), seq, getpid())) < 0) {
		err = errno;
		DEBUG_WARNING("list: read from socket failed (%m)\n", err);
		goto out;
	}

	/* Parse and print the response */
	for (;NLMSG_OK(nlmsg,(uint32_t)len);nlmsg = NLMSG_NEXT(nlmsg,len)) {
		struct net_rt rt;

		memset(&rt, 0, sizeof(struct net_rt));
		if (0 != rt_parse(nlmsg, &rt))
			continue;

#ifdef HAVE_INET6
		if (AF_INET6 == sa_af(&rt.dst)
		    && IN6_IS_ADDR_UNSPECIFIED(&rt.dst.u.in6.sin6_addr))
			continue;
#endif

		if (rth(rt.ifname, &rt.dst, rt.dstlen, &rt.gw, arg))
			break;
	}

 out:
	(void)close(sock);

	return err;
}
