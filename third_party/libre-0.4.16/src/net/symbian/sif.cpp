/**
 * @file sif.cpp  Network interface code for Symbian OS
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <es_sock.h>
#include <in_sock.h>
#include <string.h>


extern "C" {
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>

#define DEBUG_MODULE "sif"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


static void des2buf(char *buf, size_t sz, TDesC &des)
{
	TBuf8<256> buf8;
	buf8.Copy(des);

	strncpy(buf, (char *)buf8.PtrZ(), sz);
}


int net_if_debug(struct re_printf *pf, void *unused)
{
	RSocketServ serv;
	RSocket sock;
	int ret, err = 0;

	(void)unused;

	err |= re_hprintf(pf, "net interfaces:\n");

	ret = serv.Connect();
	if (KErrNone != ret)
		return kerr2errno(ret);
	ret = sock.Open(serv, KAfInet, KSockStream, KProtocolInetTcp);
	if (KErrNone != ret) {
		serv.Close();
		return kerr2errno(ret);
	}
	sock.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);

	TPckgBuf<TSoInetInterfaceInfo> ifinfo;

	while (sock.GetOpt(KSoInetNextInterface, KSolInetIfCtrl,
			   ifinfo)==KErrNone) {
		struct sa sa;
		char buf[128];
		const char *state = NULL;

		des2buf(buf, sizeof(buf), ifinfo().iName);
		err |= re_hprintf(pf, "Interface Name: %s\n", buf);

		switch (ifinfo().iState) {

		case EIfPending: state = "Pending"; break;
		case EIfUp:      state = "Up";      break;
		case EIfBusy:    state = "Busy";    break;
		case EIfDown:    state = "Down";    break;
		}
		err |= re_hprintf(pf, "  State:        %s\n", state);

		err |= re_hprintf(pf, "  Speed Metric: %d\n",
				   ifinfo().iSpeedMetric);

		TBuf<32> mac;
		mac.Zero();
		for (TUint i = sizeof(SSockAddr);
		     i<sizeof(SSockAddr)+6;
		     i++) {
			if (i < (TUint)ifinfo().iHwAddr.Length()) {
				mac.AppendFormat(_L("%02X%c"),
						 ifinfo().iHwAddr[i],
						 i<(sizeof(SSockAddr)+5)
						 ?':':' ');
			}
		}

		des2buf(buf, sizeof(buf), mac);
		err |= re_hprintf(pf, "  Mac Address:  %s\n", buf);

		sa_set_in(&sa, ifinfo().iAddress.Address(), 0);
		err |= re_hprintf(pf, "  IP Address:   %j\n", &sa);

		sa_set_in(&sa, ifinfo().iNetMask.Address(), 0);
		err |= re_hprintf(pf, "  Netmask:      %j\n", &sa);

		sa_set_in(&sa, ifinfo().iBrdAddr.Address(), 0);
		err |= re_hprintf(pf, "  Broadcast:    %s\n", &sa);

		sa_set_in(&sa, ifinfo().iDefGate.Address(), 0);
		err |= re_hprintf(pf, "  Gateway:      %s\n", &sa);

		sa_set_in(&sa, ifinfo().iNameSer1.Address(), 0);
		err |= re_hprintf(pf, "  DNS IP 1:     %s\n", &sa);

		sa_set_in(&sa, ifinfo().iNameSer2.Address(), 0);
		err |= re_hprintf(pf, "  DNS IP 2:     %s\n", &sa);
	}

	sock.Close();
	serv.Close();

	return err;
}


int net_if_list(net_ifaddr_h *ifh, void *arg)
{
	RSocketServ serv;
	RSocket sock;
	int ret;

	if (!ifh)
		return EINVAL;

	ret = serv.Connect();
	if (KErrNone != ret)
		return kerr2errno(ret);
	ret = sock.Open(serv, KAfInet, KSockStream, KProtocolInetTcp);
	if (KErrNone != ret) {
		serv.Close();
		return kerr2errno(ret);
	}
	sock.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);

	TPckgBuf<TSoInetInterfaceInfo> ifinfo;
	while (sock.GetOpt(KSoInetNextInterface, KSolInetIfCtrl,
			   ifinfo)==KErrNone) {
		struct sa sa;
		char ifname[128];

		if (EIfUp != ifinfo().iState)
			continue;

		des2buf(ifname, sizeof(ifname), ifinfo().iName);
		sa_set_in(&sa, ifinfo().iAddress.Address(), 0);

		if (ifh(ifname, &sa, arg))
			break;
	}

	sock.Close();
	serv.Close();

	return 0;
}
