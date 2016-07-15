#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

ifeq ($(USE_OPENSSL),)
SRCS	+= md5/md5.c
endif

SRCS	+= md5/wrap.c
