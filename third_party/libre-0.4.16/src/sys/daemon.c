/**
 * @file daemon.c  Daemonize process
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <re_types.h>
#include <re_mbuf.h>
#include <re_sys.h>


/**
 * Daemonize process
 *
 * @return 0 if success, otherwise errorcode
 */
int sys_daemon(void)
{
#ifdef HAVE_FORK
	pid_t pid;

	pid = fork();
	if (-1 == pid)
		return errno;
	else if (pid > 0)
		exit(0);

	if (-1 == setsid())
		return errno;

	(void)signal(SIGHUP, SIG_IGN);

	pid = fork();
	if (-1 == pid)
		return errno;
	else if (pid > 0)
		exit(0);

	if (-1 == chdir("/"))
		return errno;
	(void)umask(0);

	/* Redirect standard files to /dev/null */
	if (freopen("/dev/null", "r", stdin) == NULL)
		return errno;
	if (freopen("/dev/null", "w", stdout) == NULL)
		return errno;
	if (freopen("/dev/null", "w", stderr) == NULL)
		return errno;

	return 0;
#else
	return ENOSYS;
#endif
}
