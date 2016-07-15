/**
 * @file net/sock.c  Networking sockets code
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_net.h>


#define DEBUG_MODULE "netsock"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static bool inited = false;


#ifdef WIN32
static int wsa_init(void)
{
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int err;

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		DEBUG_WARNING("Could not load winsock (%m)\n", err);
		return err;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	if (LOBYTE(wsaData.wVersion) != 2 ||
	    HIBYTE(wsaData.wVersion) != 2 ) {
		WSACleanup();
		DEBUG_WARNING("Bad winsock verion (%d.%d)\n",
			      HIBYTE(wsaData.wVersion),
			      LOBYTE(wsaData.wVersion));
		return EINVAL;
	}

	return 0;
}
#endif


/**
 * Initialise network sockets
 *
 * @return 0 if success, otherwise errorcode
 */
int net_sock_init(void)
{
	int err = 0;

	DEBUG_INFO("sock init: inited=%d\n", inited);

	if (inited)
		return 0;

#ifdef WIN32
	err = wsa_init();
#endif

	inited = true;

	return err;
}


/**
 * Cleanup network sockets
 */
void net_sock_close(void)
{
#ifdef WIN32
	const int err = WSACleanup();
	if (0 != err) {
		DEBUG_WARNING("sock close: WSACleanup (%d)\n", err);
	}
#endif

	inited = false;

	DEBUG_INFO("sock close\n");
}
