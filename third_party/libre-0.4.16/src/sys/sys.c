/**
 * @file sys.c  System information
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdlib.h>
#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_sys.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SETRLIMIT
#include <sys/resource.h>
#endif


/**
 * Get system release version
 *
 * @param rel   Binary encoded release
 * @param maj   Major version number
 * @param min   Minor version number
 * @param patch Patch number
 *
 * @return 0 if success, otherwise errorcode
 */
int sys_rel_get(uint32_t *rel, uint32_t *maj, uint32_t *min, uint32_t *patch)
{
#ifdef HAVE_UNAME
	struct utsname u;
	struct pl pl_mj, pl_mn, pl_p;
	uint32_t mj, mn, p;
	int err;

	if (0 != uname(&u))
		return errno;

	err = re_regex(u.release, strlen(u.release),
		       "[0-9]+.[0-9]+[.\\-]1[0-9]+",
		       &pl_mj, &pl_mn, NULL, &pl_p);
	if (err)
		return err;

	mj = pl_u32(&pl_mj);
	mn = pl_u32(&pl_mn);
	p  = pl_u32(&pl_p);

	if (rel)
		*rel = mj<<16 | mn<<8 | p;
	if (maj)
		*maj = mj;
	if (min)
		*min = mn;
	if (patch)
		*patch = p;

	return 0;
#else
	(void)rel;
	(void)maj;
	(void)min;
	(void)patch;
	return EINVAL;
#endif
}


/**
 * Get kernel name and version
 *
 * @param pf     Print function for output
 * @param unused Unused parameter
 *
 * @return 0 if success, otherwise errorcode
 */
int sys_kernel_get(struct re_printf *pf, void *unused)
{
#ifdef HAVE_UNAME
	struct utsname u;

	(void)unused;

	if (0 != uname(&u))
		return errno;

	return re_hprintf(pf, "%s %s %s %s %s", u.sysname, u.nodename,
			  u.release, u.version, u.machine);
#else
	const char *str;

	(void)unused;

#if defined (__SYMBIAN32__)
	str = "Symbian OS";
#elif defined(WIN32)
	str = "Win32";
#else
	str = "?";
#endif

	return re_hprintf(pf, "%s", str);
#endif
}


/**
 * Get build info
 *
 * @param pf     Print function for output
 * @param unused Unused parameter
 *
 * @return 0 if success, otherwise errorcode
 */
int sys_build_get(struct re_printf *pf, void *unused)
{
	const unsigned int bus_width = 8*sizeof(void *);
	const char *endian = "unknown";

	const uint32_t a = 0x12345678;
	const uint8_t b0 = ((uint8_t *)&a)[0];
	const uint8_t b1 = ((uint8_t *)&a)[1];
	const uint8_t b2 = ((uint8_t *)&a)[2];
	const uint8_t b3 = ((uint8_t *)&a)[3];

	(void)unused;

	if (0x12==b0 && 0x34==b1 && 0x56==b2 && 0x78==b3)
		endian = "big";
	else if (0x12==b3 && 0x34==b2 && 0x56==b1 && 0x78==b0)
		endian = "little";

	return re_hprintf(pf, "%u-bit %s endian", bus_width, endian);
}


/**
 * Get architecture
 *
 * @return Architecture string
 */
const char *sys_arch_get(void)
{
#ifdef ARCH
	return ARCH;
#else
	return "?";
#endif
}


/**
 * Get name of Operating System
 *
 * @return Operating System string
 */
const char *sys_os_get(void)
{
#ifdef OS
	return OS;
#else
	return "?";
#endif
}


/**
 * Get libre version
 *
 * @return libre version string
 */
const char *sys_libre_version_get(void)
{
#ifdef VERSION
	return VERSION;
#else
	return "?";
#endif
}


/**
 * Return the username (login name) for the current user
 *
 * @return Username or NULL if not available
 */
const char *sys_username(void)
{
#ifdef HAVE_PWD_H
	char *login;

	login = getenv("LOGNAME");
	if (!login)
		login = getenv("USER");
#if defined (HAVE_UNISTD_H) && !defined (__SYMBIAN32__)
	if (!login) {
		login = getlogin();
	}
#endif

	return str_isset(login) ? login : NULL;
#else
	return NULL;
#endif
}


/**
 * Enable or disable coredump
 *
 * @param enable true to enable, false to disable coredump
 *
 * @return 0 if success, otherwise errorcode
 */
int sys_coredump_set(bool enable)
{
#ifdef HAVE_SETRLIMIT
	const struct rlimit rlim = {
		enable ? RLIM_INFINITY : 0,
		enable ? RLIM_INFINITY : 0
	};

	return 0 == setrlimit(RLIMIT_CORE, &rlim) ? 0 : errno;
#else
	(void)enable;
	return ENOSYS;
#endif
}
