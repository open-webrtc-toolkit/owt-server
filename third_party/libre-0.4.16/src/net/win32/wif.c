/**
 * @file wif.c  Windows network interface code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <winsock2.h>
#include <iphlpapi.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_net.h>
#include <re_sa.h>


#define DEBUG_MODULE "wif"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * List interfaces using GetAdaptersAddresses, which handles both
 * IPv4 and IPv6 address families.
 *
 * This is available from Windows XP and Windows Server 2003
 */
static int if_list_gaa(net_ifaddr_h *ifh, void *arg)
{
	IP_ADAPTER_ADDRESSES addrv[16], *cur;
	ULONG ret, len = sizeof(addrv);
	const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;
	HANDLE hLib;
	union {
		FARPROC proc;
		ULONG (WINAPI *gaa)(ULONG, ULONG, PVOID,
				    PIP_ADAPTER_ADDRESSES, PULONG);
	} u;
	bool stop = false;
	int err = 0;

	hLib = LoadLibrary(TEXT("iphlpapi.dll"));
	if (!hLib)
		return ENOSYS;

	u.proc = GetProcAddress(hLib, TEXT("GetAdaptersAddresses"));
	if (!u.proc) {
		err = ENOSYS;
		goto out;
	}

	ret = (*u.gaa)(AF_UNSPEC, flags, NULL, addrv, &len);
	if (ret != ERROR_SUCCESS) {
		DEBUG_WARNING("if_list: GetAdaptersAddresses ret=%u\n", ret);
		err = ENODEV;
		goto out;
	}

	for (cur = addrv; cur && !stop; cur = cur->Next) {
		PIP_ADAPTER_UNICAST_ADDRESS ip;

		/* an interface can have many IP-addresses */
		for (ip = cur->FirstUnicastAddress; ip; ip = ip->Next) {
			struct sa sa;

			sa_set_sa(&sa, ip->Address.lpSockaddr);

			if (ifh && ifh(cur->AdapterName, &sa, arg)) {
				stop = true;
				break;
			}
		}
	}

 out:
	FreeLibrary(hLib);

	return err;
}


/**
 * List interfaces using GetAdaptersInfo, which handles only IPv4 family.
 *
 * This is available from Windows 2000, and also works under Wine.
 */
static int if_list_gai(net_ifaddr_h *ifh, void *arg)
{
	IP_ADAPTER_INFO info[32];
	PIP_ADAPTER_INFO p = info;
	ULONG ulOutBufLen = sizeof(info);
	DWORD ret;

	ret = GetAdaptersInfo(info, &ulOutBufLen);
	if (ret != ERROR_SUCCESS) {
		DEBUG_WARNING("if_list: GetAdaptersInfo ret=%u\n", ret);
		return ENODEV;
	}

	for (p = info; p; p = p->Next) {
		struct sa sa;

		if (sa_set_str(&sa, p->IpAddressList.IpAddress.String, 0))
			continue;

		if (ifh && ifh(p->AdapterName, &sa, arg))
			break;
	}

	return 0;
}


/**
 * Enumerate all network interfaces
 *
 * @param ifh Interface handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int net_if_list(net_ifaddr_h *ifh, void *arg)
{
	/* Try both methods .. */

	if (!if_list_gaa(ifh, arg))
		return 0;

	return if_list_gai(ifh, arg);
}
