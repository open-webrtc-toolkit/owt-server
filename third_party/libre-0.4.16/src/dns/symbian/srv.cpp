/**
 * @file srv.cpp  Get DNS Server IP code for Symbian OS
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <es_sock.h>
#include <in_sock.h>


extern "C" {
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_net.h>
#include <re_dns.h>
extern int get_symbiandns(struct sa *nsv, uint32_t *n);
}


int get_symbiandns(struct sa *nsv, uint32_t *n)
{
	RSocketServ serv;
	RSocket sock;
	uint32_t i = 0;
	int ret;

	if (!nsv || !n || !*n)
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

		if (EIfUp != ifinfo().iState)
			continue;

		sa_set_in(&sa, ifinfo().iAddress.Address(), 0);

		if (sa_is_loopback(&sa))
			continue;

		if (sa_is_linklocal(&sa))
			continue;

		if (ifinfo().iNameSer1.Address()) {
			sa_set_in(&nsv[i], ifinfo().iNameSer1.Address(),
				  DNS_PORT);
			++i;
		}

		if (i >= *n)
			break;

		if (ifinfo().iNameSer2.Address()) {
			sa_set_in(&nsv[i], ifinfo().iNameSer2.Address(),
				  DNS_PORT);
			++i;
		}

		if (i >= *n)
			break;
	}

	sock.Close();
	serv.Close();

	if (i == 0)
		return ENOENT;

	*n = i;

	return 0;
}
