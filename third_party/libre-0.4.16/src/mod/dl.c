/**
 * @file dl.c  Interface to dynamic linking loader
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <dlfcn.h>
#include <re_types.h>
#include <re_fmt.h>
#include "mod_internal.h"


#define DEBUG_MODULE "dl"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

#ifdef CYGWIN
#define RTLD_LOCAL   0
#endif

static const int dl_flag = RTLD_NOW | RTLD_LOCAL;


/**
 * Load a dynamic library file
 *
 * @param name Name of library to load
 *
 * @return Opaque library handle, NULL if not loaded
 */
void *_mod_open(const char *name)
{
	void *h;

	if (!name)
		return NULL;

	h = dlopen(name, dl_flag);
	if (!h) {
		DEBUG_WARNING("mod: %s (%s)\n", name, dlerror());
		return NULL;
	}

	return h;
}


/**
 * Resolve address of symbol in dynamic library
 *
 * @param h      Library handle
 * @param symbol Name of symbol to resolve
 *
 * @return Address, NULL if failure
 */
void *_mod_sym(void *h, const char *symbol)
{
	void *sym;
	const char *err;

	if (!h || !symbol)
		return NULL;

	(void)dlerror(); /* Clear any existing error */

	sym = dlsym(h, symbol);
	err = dlerror();
	if (err)  {
		DEBUG_WARNING("dlsym: %s\n", err);
		return NULL;
	}

	return sym;
}


/**
 * Unload a dynamic library
 *
 * @param h Library handle
 */
void _mod_close(void *h)
{
	int err;

	if (!h)
		return;

	err = dlclose(h);
	if (0 != err) {
		DEBUG_WARNING("dlclose: %d\n", err);
	}
}
