#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= main/init.c
SRCS	+= main/main.c
SRCS	+= main/method.c

ifneq ($(HAVE_EPOLL),)
SRCS	+= main/epoll.c
endif

ifneq ($(USE_OPENSSL),)
SRCS    += main/openssl.c
endif
