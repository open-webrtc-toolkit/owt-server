/**
 * @file pipe.c Pipe-emulation for Windows
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <winsock2.h>
#include <re_types.h>
#include "../mqueue.h"


/** Emulate pipe on Windows -- pipe() with select() is not working */
int pipe(int fds[2])
{
	SOCKET s, rd, wr;
	struct sockaddr_in serv_addr;
	int len = sizeof(serv_addr);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return ENOSYS;

	memset((void *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(0);
	serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(s, (SOCKADDR *) & serv_addr, len) == SOCKET_ERROR)
		goto error;

	if (listen(s, 1) == SOCKET_ERROR)
		goto error;

	if (getsockname(s, (SOCKADDR *) &serv_addr, &len) == SOCKET_ERROR)
		goto error;

	if ((wr = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		goto error;

	if (connect(wr, (SOCKADDR *) &serv_addr, len) == SOCKET_ERROR) {
		closesocket(wr);
		goto error;
	}

	rd = accept(s, (SOCKADDR *) &serv_addr, &len);
	if (rd == INVALID_SOCKET) {
		closesocket(wr);
		goto error;
	}

	fds[0] = rd;
	fds[1] = wr;

	closesocket(s);
	return 0;

error:
	closesocket(s);
	return ENOSYS;
}


ssize_t pipe_read(int s, void *buf, size_t len)
{
	int ret = recv(s, buf, len, 0);

	if (ret < 0 && WSAGetLastError() == WSAECONNRESET)
		ret = 0;

	return ret;
}


ssize_t pipe_write(int s, const void *buf, size_t len)
{
	return send(s, buf, len, 0);
}

