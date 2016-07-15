/**
 * @file mqueue.h Thread Safe Message Queue -- Internal API
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifdef WIN32
int pipe(int fds[2]);
ssize_t pipe_read(int s, void *buf, size_t len);
ssize_t pipe_write(int s, const void *buf, size_t len);
#else
static inline ssize_t pipe_read(int s, void *buf, size_t len)
{
	return read(s, buf, len);
}


static inline ssize_t pipe_write(int s, const void *buf, size_t len)
{
	return write(s, buf, len);
}
#endif
