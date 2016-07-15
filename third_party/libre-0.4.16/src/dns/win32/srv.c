/**
 * @file win32/srv.c  Get DNS Server IP code for Windows
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <winsock2.h>
#include <iphlpapi.h>
#include <io.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_dns.h>
#include "../dns.h"


#define DEBUG_MODULE "win32/srv"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int get_windns(char *domain, size_t dsize, struct sa *srvv, uint32_t *n)
{
	FIXED_INFO *     FixedInfo = NULL;
	ULONG            ulOutBufLen;
	DWORD            dwRetVal;
	IP_ADDR_STRING * pIPAddr;
	HANDLE           hLib;
	union {
		FARPROC proc;
		DWORD (WINAPI *_GetNetworkParams)(FIXED_INFO*, DWORD*);
	} u;
	uint32_t i;
	int err;

	if (!srvv || !n || !*n)
		return EINVAL;

	hLib = LoadLibrary(TEXT("iphlpapi.dll"));
	if (!hLib)
		return ENOSYS;

	u.proc = GetProcAddress(hLib, TEXT("GetNetworkParams"));
	if (!u.proc) {
		err = ENOSYS;
		goto out;
	}

	FixedInfo = (FIXED_INFO *)GlobalAlloc(GPTR, sizeof( FIXED_INFO ));
	ulOutBufLen = sizeof( FIXED_INFO );

	if (ERROR_BUFFER_OVERFLOW == (*u._GetNetworkParams)(FixedInfo,
							    &ulOutBufLen)) {
		GlobalFree( FixedInfo );
		FixedInfo = (FIXED_INFO *)GlobalAlloc(GPTR, ulOutBufLen);
	}

	if ((dwRetVal = (*u._GetNetworkParams)( FixedInfo, &ulOutBufLen ))) {
		DEBUG_WARNING("couldn't get network params (%d)\n", dwRetVal);
		err = ENOENT;
		goto out;
	}

	str_ncpy(domain, FixedInfo->DomainName, dsize);

#if 0
	printf( "Host Name: %s\n", FixedInfo->HostName);
	printf( "Domain Name: %s\n", FixedInfo->DomainName);
	printf( "DNS Servers:\n" );
	printf( "\t%s\n", FixedInfo->DnsServerList.IpAddress.String );
#endif

	i = 0;
	pIPAddr = &FixedInfo->DnsServerList;
	while (pIPAddr && strlen(pIPAddr->IpAddress.String) > 0) {
		err = sa_set_str(&srvv[i], pIPAddr->IpAddress.String,
				 DNS_PORT);
		if (err) {
			DEBUG_WARNING("sa_set_str: %s (%m)\n",
				      pIPAddr->IpAddress.String, err);
		}
		DEBUG_INFO("dns ip %u: %j\n", i, &srvv[i]);
		++i;
		pIPAddr = pIPAddr ->Next;

		if (i >= *n)
			break;
	}

	*n = i;
	DEBUG_INFO("got %u nameservers\n", i);
	err = i>0 ? 0 : ENOENT;

 out:
	if (FixedInfo)
		GlobalFree(FixedInfo);
	FreeLibrary(hLib);
	return err;
}
