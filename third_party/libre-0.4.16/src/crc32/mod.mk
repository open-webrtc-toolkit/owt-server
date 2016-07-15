#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

ifeq ($(USE_ZLIB),)
SRCS	+= crc32/crc32.c
endif
