/**
 * @file netstr.c  Network strings
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_net.h>


/**
 * Get the name of a protocol
 *
 * @param proto Protocol
 *
 * @return Protocol name
 */
const char *net_proto2name(int proto)
{
	switch (proto) {

	case IPPROTO_UDP:     return "UDP";
	case IPPROTO_TCP:     return "TCP";
#ifdef IPPROTO_SCTP
	case IPPROTO_SCTP:    return "SCTP";
#endif
	default:              return "???";
	}
}


/**
 * Get the name of a address family
 *
 * @param af Address family
 *
 * @return Address family name
 */
const char *net_af2name(int af)
{
	switch (af) {

	case AF_UNSPEC:    return "AF_UNSPEC";
	case AF_INET:      return "AF_INET";
#ifdef HAVE_INET6
	case AF_INET6:     return "AF_INET6";
#endif
	default:           return "???";
	}
}
