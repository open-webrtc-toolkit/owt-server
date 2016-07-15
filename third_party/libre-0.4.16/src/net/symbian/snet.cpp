/**
 * @file snet.cpp  Networking code for Symbian OS
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <es_sock.h>
#include <in_sock.h>
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>


extern "C" {
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>

#define DEBUG_MODULE "snet"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


static struct sa local_ip;


/**
 * Convert from Symbian error code to POSIX error code
 *
 * @param kerr Symbian error code
 *
 * @return POSIX error code
 */
int kerr2errno(int kerr)
{
	switch (kerr) {

	case KErrNone:           return 0;
	case KErrNotFound:       return ENOENT;
	case KErrGeneral:        return EINVAL;
	case KErrNoMemory:       return ENOMEM;
	case KErrNotSupported:   return ENOSYS;
	case KErrArgument:       return EINVAL;
	case KErrAlreadyExists:  return EALREADY;
	case KErrTimedOut:       return ETIMEDOUT;
	case KErrNetUnreach:     return ENONET;
	case KErrHostUnreach:    return ECONNREFUSED;
	default:                 return EINVAL;
	}
}


/**
 * Get the local IP address of the device
 *
 * @note Requires at least one IP packet sent in advance!
 */
int net_if_getaddr4(const char *ifname, int af, struct sa *ip)
{
	(void)ifname;

	if (AF_INET != af)
		return EAFNOSUPPORT;

	/* Already cached? */
	if (sa_isset(&local_ip, SA_ADDR)) {
		sa_cpy(ip, &local_ip);
		return 0;
	}

	RSocketServ ss;
	RSocket s;
	TInt ret;

	ret = ss.Connect();
	if (KErrNone != ret) {
		DEBUG_WARNING("connecting to socket server fail (ret=%d)\n",
			      ret);
		return ECONNREFUSED;
	}

	ret = s.Open(ss, KAfInet, KSockDatagram, KProtocolInetUdp);
	if (KErrNone != ret) {
		DEBUG_WARNING("open socket failed (ret=%d)\n", ret);
		return ECONNREFUSED;
	}

	TInetAddr bind;
	bind.SetPort(0);
	bind.SetAddress(KInetAddrAny);

	ret = s.Bind(bind);
	if (KErrNone != ret) {
		DEBUG_WARNING("bind socket failed (ret=%d)\n", ret);
		return ECONNREFUSED;
	}

	TInetAddr local;
	s.LocalName(local);

	s.Close();
	ss.Close();

	sa_set_in(&local_ip, local.Address(), local.Port());

	DEBUG_NOTICE("local IP addr: %j\n", &local_ip);

	if (!sa_isset(&local_ip, SA_ADDR))
		return EINVAL;

	sa_cpy(ip, &local_ip);

	return 0;
}
