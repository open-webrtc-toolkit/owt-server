#
# modules.mk
#
# Copyright (C) 2010 Creytiv.com
#


# Default is enabled
MOD_AUTODETECT := 1

ifneq ($(MOD_AUTODETECT),)

ifneq ($(OS),win32)

USE_DTLS := $(shell [ -f $(SYSROOT)/include/openssl/dtls1.h ] || \
	[ -f $(SYSROOT)/local/include/openssl/dtls1.h ] || \
	[ -f $(SYSROOT_ALT)/include/openssl/dtls1.h ] && echo "yes")
USE_DTLS_SRTP := $(shell [ -f $(SYSROOT)/include/openssl/srtp.h ] || \
	[ -f $(SYSROOT)/local/include/openssl/srtp.h ] || \
	[ -f $(SYSROOT_ALT)/include/openssl/srtp.h ] && echo "yes")
USE_SRTP := $(shell [ -f $(SYSROOT)/include/srtp/srtp.h ] || \
	[ -f $(SYSROOT_ALT)/include/srtp/srtp.h ] || \
	[ -f $(SYSROOT)/local/include/srtp/srtp.h ] && echo "yes")
endif


endif

# ------------------------------------------------------------------------- #

#MODULES   += $(EXTRA_MODULES)
#MODULES   += stun turn ice

#ifneq ($(USE_SRTP),)
#MODULES   += srtp
#ifneq ($(USE_DTLS_SRTP),)
#MODULES   += dtls_srtp
#endif
#endif

