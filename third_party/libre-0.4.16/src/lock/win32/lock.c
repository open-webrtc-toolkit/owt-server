/**
 * @file win32/lock.c  Locking for Windows
 *
 * Copyright (C) 2010 Creytiv.com
 */
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_lock.h>


struct lock {
	CRITICAL_SECTION c;
};


static void lock_destructor(void *data)
{
	struct lock *l = data;

	DeleteCriticalSection(&l->c);
}


int lock_alloc(struct lock **lp)
{
	struct lock *l;

	if (!lp)
		return EINVAL;

	l = mem_alloc(sizeof(*l), lock_destructor);
	if (!l)
		return ENOMEM;

	InitializeCriticalSection(&l->c);

	*lp = l;
	return 0;
}


void lock_read_get(struct lock *l)
{
	EnterCriticalSection(&l->c);
}


void lock_write_get(struct lock *l)
{
	EnterCriticalSection(&l->c);
}


int lock_read_try(struct lock *l)
{
	return TryEnterCriticalSection(&l->c) ? 0 : ENODEV;
}


int lock_write_try(struct lock *l)
{
	return TryEnterCriticalSection(&l->c) ? 0 : ENODEV;
}


void lock_rel(struct lock *l)
{
	LeaveCriticalSection(&l->c);
}
