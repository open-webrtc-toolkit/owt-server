/**
 * @file rlib.cpp  Dynamic library loading for Symbian
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <e32std.h>
#include <re_types.h>
#include "../mod_internal.h"


extern "C" {
#define DEBUG_MODULE "rlib"
#define DEBUG_LEVEL 5
#include <re_dbg.h>
}


void *_mod_open(const char *name)
{
	TBuf<128> buf;
	TBuf8<128> buf8;
	buf8.Copy((unsigned char *)name);
	buf.Copy(buf8);

	DEBUG_INFO("rlib open (%s)\n", name);

	RLibrary *lib = new RLibrary;
	if (!lib) {
		DEBUG_WARNING("open: could not allocate RLibrary (%s)\n",
			      name);
		return NULL;
	}
	TInt err = lib->Load(buf);
	if (KErrNone != err) {
		DEBUG_WARNING("open: Load failed with err=%d (%s)\n", err,
			      name);
		delete lib;
		return NULL;
	}

	return lib;
}


void *_mod_sym(void *h, const char *symbol)
{
	RLibrary *lib = (RLibrary *)h;

	(void)symbol;

	/* NOTE: symbol lookup is done by ordinal.
	 */
	TLibraryFunction func = lib->Lookup(1);
	TInt ret = func();

	DEBUG_INFO("rlib sym: ret=0x%08x\n", func, ret);

	return (void *)ret;
}


void _mod_close(void *h)
{
	RLibrary *lib = (RLibrary *)h;

	delete lib;
}
