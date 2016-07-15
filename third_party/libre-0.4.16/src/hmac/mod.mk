#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= hmac/hmac_sha1.c

ifneq ($(USE_OPENSSL),)
SRCS	+= hmac/openssl/hmac.c
else ifneq ($(USE_APPLE_COMMONCRYPTO),)
SRCS	+= hmac/apple/hmac.c
else
SRCS	+= hmac/hmac.c
endif
