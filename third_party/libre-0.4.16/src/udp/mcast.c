/**
 * @file mcast.c  UDP Multicast
 *
 * Copyright (C) 2010 Creytiv.com
 */

#define _BSD_SOURCE 1
#define _DEFAULT_SOURCE 1
#include <re_types.h>
#include <re_fmt.h>
#include <re_sa.h>
#include <re_udp.h>


static int multicast_update(struct udp_sock *us, const struct sa *group,
			    bool join)
{
	struct ip_mreq mreq;
#ifdef HAVE_INET6
	struct ipv6_mreq mreq6;
#endif
	int err;

	if (!us || !group)
		return EINVAL;

	switch (sa_af(group)) {

	case AF_INET:
		mreq.imr_multiaddr = group->u.in.sin_addr;
		mreq.imr_interface.s_addr = 0;

		err = udp_setsockopt(us, IPPROTO_IP,
				     join
				     ? IP_ADD_MEMBERSHIP
				     : IP_DROP_MEMBERSHIP,
				     &mreq, sizeof(mreq));
		break;

#ifdef HAVE_INET6
	case AF_INET6:
		mreq6.ipv6mr_multiaddr = group->u.in6.sin6_addr;
		mreq6.ipv6mr_interface = 0;

		err = udp_setsockopt(us, IPPROTO_IPV6,
				     join
				     ? IPV6_JOIN_GROUP
				     : IPV6_LEAVE_GROUP,
				     &mreq6, sizeof(mreq6));
		break;
#endif

	default:
		return EAFNOSUPPORT;
	}

	return err;
}


int udp_multicast_join(struct udp_sock *us, const struct sa *group)
{
	return multicast_update(us, group, true);
}


int udp_multicast_leave(struct udp_sock *us, const struct sa *group)
{
	return multicast_update(us, group, false);
}
