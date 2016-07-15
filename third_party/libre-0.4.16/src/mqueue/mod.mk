#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= mqueue/mqueue.c

ifeq ($(OS),win32)
SRCS	+= mqueue/win32/pipe.c
endif
