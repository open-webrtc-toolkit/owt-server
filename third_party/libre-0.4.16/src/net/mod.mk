#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

# Generic files
SRCS	+= net/if.c
SRCS	+= net/net.c
SRCS	+= net/netstr.c
SRCS	+= net/rt.c
SRCS	+= net/sock.c
SRCS	+= net/sockopt.c


# Platform dependant files
ifneq ($(OS),win32)
ifneq ($(OS),cygwin)
SRCS	+= net/posix/pif.c
endif
else
SRCS	+= net/win32/wif.c
endif


# Routing
ifeq ($(OS),linux)
SRCS	+= net/linux/rt.c
CFLAGS  += -DHAVE_ROUTE_LIST
else

ifneq ($(HAVE_SYS_SYSCTL_H),)
ifneq ($(HAVE_NET_ROUTE_H),)
SRCS	+= net/bsd/brt.c
CFLAGS  += -DHAVE_ROUTE_LIST
endif
endif

endif

ifdef HAVE_GETIFADDRS
SRCS	+= net/ifaddrs.c
endif
