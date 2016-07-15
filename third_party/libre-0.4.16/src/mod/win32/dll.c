/**
 * @file dll.c  Dynamic library loading for Windows
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <windows.h>
#include <re_types.h>
#include "../mod_internal.h"


#define DEBUG_MODULE "dll"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/**
 * Open a DLL file
 *
 * @param name  Name of DLL to open
 *
 * @return Handle (NULL if failed)
 */
void *_mod_open(const char *name)
{
	HINSTANCE DllHandle = 0;

	DEBUG_INFO("loading %s\n", name);

	DllHandle = LoadLibraryA(name);
	if (!DllHandle) {
		DEBUG_WARNING("open: %s LoadLibraryA() failed\n", name);
		return NULL;
	}

	return DllHandle;
}


/**
 * Resolve a symbol address in a DLL
 *
 * @param h       DLL Handle
 * @param symbol  Symbol to resolve
 *
 * @return Address of symbol
 */
void *_mod_sym(void *h, const char *symbol)
{
	HINSTANCE DllHandle = (HINSTANCE)h;
	union {
		FARPROC sym;
		void *ptr;
	} u;

	if (!DllHandle)
		return NULL;

	DEBUG_INFO("get symbol: %s\n", symbol);

	u.sym = GetProcAddress(DllHandle, symbol);
	if (!u.sym) {
		DEBUG_WARNING("GetProcAddress: no symbol %s\n", symbol);
		return NULL;
	}

	return u.ptr;
}


/**
 * Close a DLL
 *
 * @param h DLL Handle
 */
void _mod_close(void *h)
{
	HINSTANCE DllHandle = (HINSTANCE)h;

	DEBUG_INFO("unloading %p\n", h);

	if (!DllHandle)
		return;

	FreeLibrary(DllHandle);
}
