#
# module.mk
#
# Copyright (C) 2010 Creytiv.com
#

MOD		:= srtp
$(MOD)_SRCS	+= srtp.c sdes.c
$(MOD)_LFLAGS	+=

include mk/mod.mk
