/**
 * @file rwlock.c  Pthread read/write locking
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _GNU_SOURCE 1
#include <pthread.h>
#include <re_types.h>
#include <re_mem.h>
#include <re_lock.h>


#define DEBUG_MODULE "rwlock"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct lock {
	pthread_rwlock_t lock;
};


static void lock_destructor(void *data)
{
	struct lock *l = data;

	int err = pthread_rwlock_destroy(&l->lock);
	if (err) {
		DEBUG_WARNING("pthread_rwlock_destroy: %m\n", err);
	}
}


int lock_alloc(struct lock **lp)
{
	struct lock *l;
	int err;

	if (!lp)
		return EINVAL;

	l = mem_zalloc(sizeof(*l), lock_destructor);
	if (!l)
		return ENOMEM;

	err = pthread_rwlock_init(&l->lock, NULL);
	if (err)
		goto out;

	*lp = l;

 out:
	if (err)
		mem_deref(l);
	return err;
}


void lock_read_get(struct lock *l)
{
	int err;

	if (!l)
		return;

	err = pthread_rwlock_rdlock(&l->lock);
	if (err) {
		DEBUG_WARNING("lock_read_get: %m\n", err);
	}
}


void lock_write_get(struct lock *l)
{
	int err;

	if (!l)
		return;

	err = pthread_rwlock_wrlock(&l->lock);
	if (err) {
		DEBUG_WARNING("lock_write_get: %m\n", err);
	}
}


int lock_read_try(struct lock *l)
{
	if (!l)
		return EINVAL;
	return pthread_rwlock_tryrdlock(&l->lock);
}


int lock_write_try(struct lock *l)
{
	if (!l)
		return EINVAL;
	return pthread_rwlock_trywrlock(&l->lock);
}


void lock_rel(struct lock *l)
{
	int err;

	if (!l)
		return;

	err = pthread_rwlock_unlock(&l->lock);
	if (err) {
		DEBUG_WARNING("lock_rel: %m\n", err);
	}
}
