/**
 * @file conf.c  Configuration utils
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _BSD_SOURCE 1
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <sys/stat.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <re.h>
#include <baresip.h>
#include "core.h"


#define DEBUG_MODULE ""
#define DEBUG_LEVEL 0
#include <re_dbg.h>


#ifdef WIN32
#define open _open
#define read _read
#define close _close
#endif


#if defined (WIN32) || defined (__SYMBIAN32__)
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif

static struct config core_config = {
	/** SIP User-Agent */
	{
		128,
		"",
                "",
                ""
	},

	/** Audio */
	{
		{8000, 48000},
		{1, 2},
	},

#ifdef USE_VIDEO
	/** Video */
	{
		352, 288,
		512000,
		30,
	},
#endif

	/** Audio/Video Transport */
	{
		0xb8,
		{10000, 20000},
		{512000, 2048000},
		true,
		false,
		{0, 0},/*{5, 10},*/
		false
	},

	/* Network */
	{
		""
	},

#ifdef USE_VIDEO
	/* BFCP */
	{
		""
	},
#endif
};


struct config *conf_config(void)
{
	return &core_config;
}


/**
 * Configure the system with default settings
 *
 * @return 0 if success, otherwise errorcode
 */
int conf_init(void)
{
    int err = 0;

	err = poll_method_set(METHOD_POLL);
	if (err) {
		warning("conf_init: poll method set: %m\n", err);
		goto out;
	}

	/* Initialise Network */
	err = net_init(&core_config.net, AF_INET/*prefer_ipv6 ? AF_INET6 : AF_INET*/);
	if (err) {
		warning("conf_init: network init failed: %m\n", err);
		goto out;
	}
out:
	return err;
}

#if 0
static int load_module(const struct pl *modpath, const struct pl *name)
{
	char file[256];
	struct mod *m = NULL;
	int err = 0;

	if (!name)
		return EINVAL;

#ifdef STATIC
	/* Try static first */
	err = mod_add(&m, find_module(name));
	if (!err)
		goto out;
#endif

	/* Then dynamic */
	if (re_snprintf(file, sizeof(file), "%r/%r", modpath, name) < 0) {
		err = ENOMEM;
		goto out;
	}
	err = mod_load(&m, file);
	if (err)
		goto out;

 out:
	if (err) {
		warning("module %r: %m\n", name, err);
	}

	return err;
}
#endif

int load_modules(void/*const char *modpath*/)
{
	int err = 0;
	/*(void)modpath;*/
/*	struct pl path;
	struct pl name;

	pl_set_str(&path, modpath);
*/
	register_audio_codecs();
	register_video_codecs();

    /*pl_set_str(&name, "stun.so");
	err = load_module(&path, &name);
	pl_set_str(&name, "turn.so");
	err |= load_module(&path, &name);
	pl_set_str(&name, "ice.so");
	err |= load_module(&path, &name);
	pl_set_str(&name, "natpmp.so");
	err |= load_module(&path, &name);
	pl_set_str(&name, "srtp.so");
	err |= load_module(&path, &name);
	pl_set_str(&name, "dtls_srtp.so");
	err |= load_module(&path, &name);*/
	if (err) {
		warning("load_modules: could not load modules: %m\n", err);
	}

	return err;
}

void unload_modules(void)
{
	mod_close();
}
