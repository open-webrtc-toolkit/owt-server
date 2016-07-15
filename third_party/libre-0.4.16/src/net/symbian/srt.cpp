/**
 * @file srt.cpp  Routing table code for Symbian OS
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <es_sock.h>
#include <in_sock.h>
#include <string.h>
#include <re_types.h>


extern "C" {
#include <re_sa.h>
#include <re_net.h>
#include <re_mbuf.h>

#define DEBUG_MODULE "netrt"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


int net_rt_list(net_rt_h *rth, void *arg)
{
	if (!rth)
		return EINVAL;

	RSocketServ serv;
	RSocket sock;
	if (KErrNone != serv.Connect())
		return ECONNREFUSED;
	if (KErrNone != sock.Open(serv, KAfInet, KSockStream,
				  KProtocolInetTcp))
		return ECONNREFUSED;
	sock.SetOpt(KSoInetEnumRoutes, KSolInetRtCtrl);

	TPckgBuf<TSoInetRouteInfo> rtinfo;
	while (sock.GetOpt(KSoInetNextRoute, KSolInetRtCtrl,
			   rtinfo)==KErrNone) {
		char ifname[16];
		struct sa dst, gw, sa, if_ip;
		int dstlen;
		int af = AF_INET;

		sa_set_in(&if_ip, rtinfo().iIfAddr.Address(), 0);

#ifdef HAVE_INET6
		if (KAfInet6 == rtinfo().iIfAddr.Family())
			af = AF_INET6;
#endif
		if (net_if_getname(ifname, sizeof(ifname), af, &if_ip)) {
			DEBUG_WARNING("  could not get ifname for IP=%j\n",
				      &if_ip);
		}

		sa_set_in(&gw, rtinfo().iGateway.Address(), 0);
		sa_set_in(&dst, rtinfo().iDstAddr.Address(), 0);
		sa_set_in(&sa, rtinfo().iNetMask.Address(), 0);

		dstlen = 0; // TODO

		if (rth(ifname, &dst, dstlen, &gw, arg))
			break;
	}

	sock.Close();
	serv.Close();

	return 0;
}


int net_rt_default_get(int af, char *ifname, size_t size)
{
	(void)af;
	(void)ifname;
	(void)size;
	return ENOSYS;
}
