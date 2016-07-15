/**
 * @file mqueue.c Thread Safe Message Queue
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <unistd.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_main.h>
#include <re_mqueue.h>
#include "mqueue.h"


#define MAGIC 0x14553399


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#endif


/**
 * Defines a Thread-safe Message Queue
 *
 * The Message Queue can be used to communicate between two threads. The
 * receiving thread must run the re_main() loop which will be woken up on
 * incoming messages from other threads. The sender thread can be any thread.
 */
struct mqueue {
	int pfd[2];
	mqueue_h *h;
	void *arg;
};

struct msg {
	int id;
	void *data;
	uint32_t magic;
};


static void destructor(void *arg)
{
	struct mqueue *q = arg;

	if (q->pfd[0] >= 0) {
		fd_close(q->pfd[0]);
		(void)close(q->pfd[0]);
	}
	if (q->pfd[1] >= 0)
		(void)close(q->pfd[1]);
}


static void event_handler(int flags, void *arg)
{
	struct mqueue *mq = arg;
	struct msg msg;
	ssize_t n;

	if (!(flags & FD_READ))
		return;

	n = pipe_read(mq->pfd[0], &msg, sizeof(msg));
	if (n < 0)
		return;

	if (n != sizeof(msg)) {
		(void)re_fprintf(stderr, "mqueue: short read of %d bytes\n",
				 n);
		return;
	}

	if (msg.magic != MAGIC) {
		(void)re_fprintf(stderr, "mqueue: bad magic on read (%08x)\n",
				 msg.magic);
		return;
	}

	mq->h(msg.id, msg.data, mq->arg);
}


/**
 * Allocate a new Message Queue
 *
 * @param mqp Pointer to allocated Message Queue
 * @param h   Message handler
 * @param arg Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int mqueue_alloc(struct mqueue **mqp, mqueue_h *h, void *arg)
{
	struct mqueue *mq;
	int err = 0;

	if (!mqp || !h)
		return EINVAL;

	mq = mem_zalloc(sizeof(*mq), destructor);
	if (!mq)
		return ENOMEM;

	mq->h   = h;
	mq->arg = arg;

	mq->pfd[0] = mq->pfd[1] = -1;
	if (pipe(mq->pfd) < 0) {
		err = errno;
		goto out;
	}

	err = fd_listen(mq->pfd[0], FD_READ, event_handler, mq);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(mq);
	else
		*mqp = mq;

	return err;
}


/**
 * Push a new message onto the Message Queue
 *
 * @param mq   Message Queue
 * @param id   General purpose Identifier
 * @param data Application data
 *
 * @return 0 if success, otherwise errorcode
 */
int mqueue_push(struct mqueue *mq, int id, void *data)
{
	struct msg msg;
	ssize_t n;

	if (!mq)
		return EINVAL;

	msg.id    = id;
	msg.data  = data;
	msg.magic = MAGIC;

	n = pipe_write(mq->pfd[1], &msg, sizeof(msg));
	if (n < 0)
		return errno;

	return (n != sizeof(msg)) ? EPIPE : 0;
}
