/**
 * @file rmutex.cpp  Symbian RMutex locking
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <e32std.h>

extern "C" {
#include <re_types.h>
#include <re_mem.h>
#include <re_lock.h>
}


struct lock {
	RMutex m;
};


static void lock_destructor(void *data)
{
	struct lock *l = (struct lock *)data;

	l->m.Close();
}


int lock_alloc(struct lock **lp)
{
	struct lock *l;

	if (!lp)
		return EINVAL;

	l = (struct lock *)mem_zalloc(sizeof(*l), lock_destructor);
	if (!l)
		return ENOMEM;

	const TInt ret = l->m.CreateLocal();
	if (KErrNone != ret) {
		return EINVAL;
	}

	*lp = l;
	return 0;
}


void lock_read_get(struct lock *l)
{
	if (!l)
		return;

	l->m.Wait();
}


void lock_write_get(struct lock *l)
{
	if (!l)
		return;

	l->m.Wait();
}


int lock_read_try(struct lock *l)
{
	(void)l;
	return ENOSYS;
}


int lock_write_try(struct lock *l)
{
	(void)l;
	return ENOSYS;
}


void lock_rel(struct lock *l)
{
	if (!l)
		return;

	l->m.Signal();
}
