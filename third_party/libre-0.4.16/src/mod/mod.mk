#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= mod/mod.c

# Unix dlopen
ifdef HAVE_DLFCN_H
SRCS	+= mod/dl.c
endif

ifeq ($(OS),win32)
SRCS	+= mod/win32/dll.c
endif
