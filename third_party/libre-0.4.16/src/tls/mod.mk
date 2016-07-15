#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

ifneq ($(USE_OPENSSL),)
SRCS	+= tls/openssl/tls.c
SRCS	+= tls/openssl/tls_tcp.c
SRCS	+= tls/openssl/tls_udp.c
endif
