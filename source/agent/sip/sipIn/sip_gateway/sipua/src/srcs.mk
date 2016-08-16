#
# srcs.mk All application source files.
#
# Copyright (C) 2010 Creytiv.com
#

SRCS    += account.c
SRCS	+= aucodec.c
SRCS	+= audio.c
SRCS	+= call.c
SRCS	+= conf.c
SRCS	+= log.c
SRCS	+= metric.c
SRCS	+= mnat.c
SRCS    += menc.c
SRCS	+= net.c
SRCS	+= realtime.c
SRCS	+= reg.c
SRCS	+= rtpkeep.c
SRCS	+= sdp.c
SRCS	+= sipreq.c
SRCS	+= stream.c
SRCS	+= ua.c
SRCS    += uag.c
SRCS    += sipua.c
SRCS    += sipua_actions.c

ifneq ($(USE_VIDEO),)
SRCS	+= bfcp.c
SRCS	+= mctrl.c
SRCS	+= video.c
SRCS	+= vidcodec.c
endif

ifneq ($(STATIC),)
SRCS	+= static.c
endif

