/**
 * @file openssl.c  OpenSSL initialisation and multi-threading routines
 *
 * Copyright (C) 2010 Creytiv.com
 */
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <re_types.h>
#include <re_lock.h>
#include <re_mem.h>
#include "main.h"


#ifdef HAVE_PTHREAD

static pthread_mutex_t *lockv;


static inline unsigned long threadid(void)
{
#if defined (DARWIN) || defined (FREEBSD) || defined (OPENBSD) || \
	defined (NETBSD)
	return (unsigned long)(void *)pthread_self();
#else
	return (unsigned long)pthread_self();
#endif
}


#if OPENSSL_VERSION_NUMBER >= 0x10000000
static void threadid_handler(CRYPTO_THREADID *id)
{
	CRYPTO_THREADID_set_numeric(id, threadid());
}
#else
static unsigned long threadid_handler(void)
{
	return threadid();
}
#endif


static void locking_handler(int mode, int type, const char *file, int line)
{
	(void)file;
	(void)line;

	if (mode & CRYPTO_LOCK)
		(void)pthread_mutex_lock(&lockv[type]);
	else
		(void)pthread_mutex_unlock(&lockv[type]);
}

#endif


static struct CRYPTO_dynlock_value *dynlock_create_handler(const char *file,
							   int line)
{
	struct lock *lock;
	(void)file;
	(void)line;

	if (lock_alloc(&lock))
		return NULL;

	return (struct CRYPTO_dynlock_value *)lock;
}


static void dynlock_lock_handler(int mode, struct CRYPTO_dynlock_value *l,
				 const char *file, int line)
{
	struct lock *lock = (struct lock *)l;
	(void)file;
	(void)line;

	if (mode & CRYPTO_LOCK)
		lock_write_get(lock);
	else
		lock_rel(lock);
}


static void dynlock_destroy_handler(struct CRYPTO_dynlock_value *l,
				    const char *file, int line)
{
	(void)file;
	(void)line;

	mem_deref(l);
}


#ifdef SIGPIPE
static void sigpipe_handler(int x)
{
	(void)x;
	(void)signal(SIGPIPE, sigpipe_handler);
}
#endif


int openssl_init(void)
{
#ifdef HAVE_PTHREAD
	int err, i;

	lockv = mem_zalloc(sizeof(pthread_mutex_t) * CRYPTO_num_locks(), NULL);
	if (!lockv)
		return ENOMEM;

	for (i=0; i<CRYPTO_num_locks(); i++) {

		err = pthread_mutex_init(&lockv[i], NULL);
		if (err) {
			lockv = mem_deref(lockv);
			return err;
		}
	}

#if OPENSSL_VERSION_NUMBER >= 0x10000000
	CRYPTO_THREADID_set_callback(threadid_handler);
#else
	CRYPTO_set_id_callback(threadid_handler);
#endif

	CRYPTO_set_locking_callback(locking_handler);
#endif

	CRYPTO_set_dynlock_create_callback(dynlock_create_handler);
	CRYPTO_set_dynlock_lock_callback(dynlock_lock_handler);
	CRYPTO_set_dynlock_destroy_callback(dynlock_destroy_handler);

#ifdef SIGPIPE
	(void)signal(SIGPIPE, sigpipe_handler);
#endif

	SSL_library_init();
	SSL_load_error_strings();

	return 0;
}


void openssl_close(void)
{
	ERR_free_strings();
#ifdef HAVE_PTHREAD
	lockv = mem_deref(lockv);
#endif
}
