/**
 * @file re_mqueue.h Thread Safe Message Queue
 *
 * Copyright (C) 2010 Creytiv.com
 */

struct mqueue;

typedef void (mqueue_h)(int id, void *data, void *arg);

int mqueue_alloc(struct mqueue **mqp, mqueue_h *h, void *arg);
int mqueue_push(struct mqueue *mq, int id, void *data);
