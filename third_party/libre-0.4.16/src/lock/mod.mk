#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

ifdef HAVE_PTHREAD_RWLOCK
SRCS	+= lock/rwlock.c
else
ifdef HAVE_PTHREAD
SRCS	+= lock/lock.c
endif
endif

ifeq ($(OS),win32)
SRCS	+= lock/win32/lock.c
endif
