#
# re.mk - common make rules
#
# Copyright (C) 2010 Creytiv.com
#
# Imported variables:
#
#   ARCH           Target architecture
#   CC             Compiler
#   CROSS_COMPILE  Cross-compiler prefix (optional)
#   EXTRA_CFLAGS   Extra compiler flags appended to CFLAGS
#   EXTRA_LFLAGS   Extra linker flags appended to LFLAGS
#   GCOV           If non-empty, enable GNU Coverage testing
#   GPROF          If non-empty, enable GNU Profiling
#   OPT_SIZE       If non-empty, optimize for size
#   OPT_SPEED      If non-empty, optimize for speed
#   PROJECT        Project name
#   RELEASE        Release build
#   SYSROOT        System root of library and include files
#   SYSROOT_ALT    Alternative system root of library and include files
#   USE_OPENSSL    If non-empty, link to libssl library
#   USE_ZLIB       If non-empty, link to libz library
#   VERSION        Version number
#
# Exported variables:
#
#   APP_LFLAGS     Linker flags for applications using modules
#   BIN_SUFFIX     Suffix for binary executables
#   CC             Compiler
#   CCACHE         Compiler ccache tool
#   CFLAGS         Compiler flags
#   DFLAGS         Dependency generator flags
#   LFLAGS         Common linker flags
#   LIBS           Libraries to link against
#   LIB_SUFFIX     Suffix for shared libraries
#   MOD_LFLAGS     Linker flags for dynamic modules
#   MOD_SUFFIX     Suffix for dynamic modules
#   SH_LFLAGS      Linker flags for shared libraries
#   USE_TLS        Defined if TLS is available
#   USE_DTLS       Defined if DTLS is available
#


ifneq ($(RELEASE),)
CFLAGS  += -DRELEASE
OPT_SPEED=1
endif


# Default system root
ifeq ($(SYSROOT),)
SYSROOT := /usr
endif

# Alternative Systemroot
ifeq ($(SYSROOT_ALT),)
SYSROOT_ALT := $(shell [ -d /sw/include ] && echo "/sw")
endif
ifeq ($(SYSROOT_ALT),)
SYSROOT_ALT := $(shell [ -d /opt/local/include ] && echo "/opt/local")
endif

ifneq ($(SYSROOT_ALT),)
CFLAGS  += -I$(SYSROOT_ALT)/include
LFLAGS  += -L$(SYSROOT_ALT)/lib
endif


##############################################################################
#
# Compiler section
#
# find compiler name & version

ifeq ($(CC),)
	CC := gcc
endif
ifeq ($(CC),cc)
	CC := gcc
endif
LD := $(CC)
CC_LONGVER := $(shell if $(CC) -v 2>/dev/null; then \
						$(CC) -v 2>&1 ;\
					else \
						$(CC) -V 2>&1 ; \
					fi )

# find-out the compiler's name

ifneq (,$(findstring gcc, $(CC_LONGVER)))
	CC_NAME := gcc
	CC_VER := $(word 1,$(CC)) $(shell $(CC) - --version|head -n 1|\
		cut -d" " -f 3|\
		sed -e 's/^.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/'\
		-e 's/^[^0-9].*\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/')
	# sun sed is a little brain damaged => this complicated expression
	MKDEP := $(CC) -MM
	#transform gcc version into 2.9x, 3.x or 4.x
	CC_SHORTVER := $(shell echo "$(CC_VER)" | cut -d" " -f 2| \
			 sed -e 's/[^0-9]*-\(.*\)/\1/'| \
			 sed -e 's/2\.9.*/2.9x/' -e 's/3\.[0-3]\..*/3.0/' -e \
			 	's/3\.[0-3]/3.0/' -e 's/3\.[4-9]\..*/3.4/' -e\
				's/3\.[4-9]/3.4/' -e 's/4\.[0-9]\..*/4.x/' -e\
				's/4\.[0-9]/4.x/' )
endif

ifeq ($(CC_NAME),)
ifneq (,$(findstring clang, $(CC_LONGVER)))
	CC_NAME := clang
	CC_SHORTVER := $(shell echo "$(CC_LONGVER)"|head -n 1| \
		sed -e 's/.*version \([0-9]\.[0-9]\).*/\1/g' )
	CC_VER := $(CC) $(CC_SHORTVER)
	MKDEP := $(CC) -MM
endif
endif

ifeq ($(CC_NAME),)
ifneq (, $(findstring Sun, $(CC_LONGVER)))
	CC_NAME := suncc
	CC_SHORTVER := $(shell echo "$(CC_LONGVER)"|head -n 1| \
					sed -e 's/.*\([0-9]\.[0-9]\).*/\1/g' )
	CC_VER := $(CC) $(CC_SHORTVER)
	MKDEP  := $(CC) -xM1
endif
endif

ifeq ($(CC_NAME),)
ifneq (, $(findstring Intel(R) C++ Compiler, $(CC_LONGVER)))
	# very nice: gcc compatible
	CC_NAME := icc
	CC_FULLVER := $(shell echo "$(CC_LONGVER)"|head -n 1| \
			sed -e 's/.*Version \([0-9]\.[0-9]\.[0-9]*\).*/\1/g')
	CC_SHORTVER := $(shell echo "$(CC_FULLVER)" | cut -d. -f1,2 )
	CC_VER := $(CC) $(CC_FULLVER)
	MKDEP  := $(CC) -MM
endif
endif


ifeq (,$(CC_NAME))
#not found
	CC_NAME     := $(CC)
	CC_SHORTVER := unknown
	CC_VER      := unknown
	MKDEP       := gcc -MM
$(warning	Unknown compiler $(CC)\; supported compilers: \
			gcc, clang, sun cc, intel icc )
endif


# Compiler warning flags
CFLAGS	+= -Wall
CFLAGS	+= -Wmissing-declarations
CFLAGS	+= -Wmissing-prototypes
CFLAGS	+= -Wstrict-prototypes
CFLAGS	+= -Wbad-function-cast
CFLAGS	+= -Wsign-compare
CFLAGS	+= -Wnested-externs
CFLAGS	+= -Wshadow
CFLAGS	+= -Waggregate-return
CFLAGS	+= -Wcast-align


ifeq ($(CC_SHORTVER),4.x)
CFLAGS	+= -Wextra
CFLAGS	+= -Wold-style-definition
CFLAGS	+= -Wdeclaration-after-statement
endif

CFLAGS  += -g
ifneq ($(OPT_SPEED),)
CFLAGS  += -O3  # Optimize for speed - takes longer to compile!
OPTIMIZE := 1
endif
ifneq ($(OPT_SIZE),)
CFLAGS  += -Os  # Optimize for size - takes longer to compile!
OPTIMIZE := 1
endif

ifneq ($(OPTIMIZE),)
CFLAGS	+= -Wuninitialized
ifneq ($(CC_SHORTVER), 2.9x)
CFLAGS	+= -Wno-strict-aliasing
endif
endif

# Compiler dependency flags
ifeq ($(CC_SHORTVER), 2.9x)
	DFLAGS		= -MD
else
	DFLAGS		= -MD -MF $(@:.o=.d) -MT $@
endif


##############################################################################
#
# OS section
#

MACHINE   := $(shell $(CC) -dumpmachine)

ifeq ($(CROSS_COMPILE),)
OS        := $(shell uname -s | sed -e s/SunOS/solaris/ | tr "[A-Z]" "[a-z]")
endif


ifneq ($(strip $(filter i386-mingw32 i486-mingw32 i586-mingw32msvc \
	i686-w64-mingw32 mingw32, \
	$(MACHINE))),)
	OS   := win32
ifeq ($(MACHINE), mingw32)
	CROSS_COMPILE :=
endif
endif


# default
LIB_SUFFIX	:= .so
MOD_SUFFIX	:= .so
BIN_SUFFIX	:=

ifeq ($(OS),solaris)
	CFLAGS		+= -fPIC -DSOLARIS
	LIBS		+= -ldl -lsocket -lnsl
	LFLAGS		+= -fPIC
	SH_LFLAGS	+= -G
	MOD_LFLAGS	+=
	APP_LFLAGS	+=
	AR		:= ar
	AFLAGS		:= cru
endif
ifeq ($(OS),linux)
	CFLAGS		+= -fPIC -DLINUX
	LIBS		+= -ldl
	LFLAGS		+= -fPIC
	SH_LFLAGS	+= -shared
	MOD_LFLAGS	+=
	APP_LFLAGS	+= -rdynamic
	AR		:= ar
	AFLAGS		:= cru
endif
ifeq ($(OS),darwin)
	CFLAGS		+= -fPIC -dynamic -DDARWIN
ifneq (,$(findstring Apple, $(CC_LONGVER)))
	CFLAGS		+= -Wshorten-64-to-32
endif
	DFLAGS		:= -MD
	LFLAGS		+= -fPIC
	SH_LFLAGS	+= -dynamiclib
ifeq ($(CC_NAME),gcc)
	SH_LFLAGS	+= -dylib
endif
ifneq ($(VERSION),)
	SH_LFLAGS	+= -current_version $(VERSION)
	SH_LFLAGS	+= -compatibility_version $(VERSION)
endif
	MOD_LFLAGS	+= -undefined dynamic_lookup
	APP_LFLAGS	+=
	AR		:= ar
	AFLAGS		:= cru
	LIB_SUFFIX	:= .dylib
	HAVE_KQUEUE	:= 1
endif
ifeq ($(OS),netbsd)
	CFLAGS		+= -fPIC -DNETBSD
	LFLAGS		+= -fPIC
	SH_LFLAGS	+= -shared
	MOD_LFLAGS	+=
	APP_LFLAGS	+= -rdynamic
	AR		:= ar
	AFLAGS		:= cru
	HAVE_KQUEUE	:= 1
endif
ifeq ($(OS),freebsd)
	CFLAGS		+= -fPIC -DFREEBSD
	LFLAGS		+= -fPIC
	SH_LFLAGS	+= -shared
	MOD_LFLAGS	+=
	APP_LFLAGS	+= -rdynamic
	AR		:= ar
	AFLAGS		:= cru
	HAVE_KQUEUE	:= 1
endif
ifeq ($(OS),openbsd)
	CFLAGS		+= -fPIC -DOPENBSD
	LFLAGS		+= -fPIC
	SH_LFLAGS	+= -shared
	MOD_LFLAGS	+=
	APP_LFLAGS	+= -rdynamic
	AR		:= ar
	AFLAGS		:= cru
	HAVE_KQUEUE	:= 1
	HAVE_ARC4RANDOM	:= 1
endif
ifeq ($(OS),win32)
	CFLAGS		+= -DWIN32 -D_WIN32_WINNT=0x0501 -D__ssize_t_defined
	LIBS		+= -lwsock32 -lws2_32 -liphlpapi
	LFLAGS		+=
	SH_LFLAGS	+= -shared
	MOD_LFLAGS	+=
	APP_LFLAGS	+= -Wl,--export-all-symbols
	AR		:= ar
	AFLAGS		:= cru
	CROSS_COMPILE	?= $(MACHINE)-
	RANLIB		:= $(CROSS_COMPILE)ranlib
	LIB_SUFFIX	:= .dll
	MOD_SUFFIX	:= .dll
	BIN_SUFFIX	:= .exe
	SYSROOT		:= /usr/$(MACHINE)/
endif
ifeq ($(OS),cygwin)
	CFLAGS		+= -DCYGWIN -D_WIN32_WINNT=0x0501
	LIBS		+= -lwsock32 -lws2_32
	LFLAGS		+=
	SH_LFLAGS	+= -shared
	MOD_LFLAGS	+=
	APP_LFLAGS	+= -Wl,-E
	AR		:= ar
	AFLAGS		:= cru
endif

CFLAGS	+= -DOS=\"$(OS)\"

ifeq ($(CC_SHORTVER),2.9x)
CFLAGS  += -Wno-long-long
else
CFLAGS  += -std=c99
PEDANTIC := 1
endif # CC_SHORTVER

ifneq ($(PEDANTIC),)
CFLAGS  += -pedantic
endif


ifeq ($(OS),)
$(warning Could not detect OS)
endif


##############################################################################
#
# Architecture section
#


ifeq ($(ARCH),)
ifeq ($(CC_NAME),$(filter $(CC_NAME),gcc clang))
PREDEF	:= $(shell $(CC) -dM -E -x c $(EXTRA_CFLAGS) $(CFLAGS) /dev/null)

ifneq ($(strip $(filter i386 __i386__ __i386 _M_IX86 __X86__ _X86_, \
	$(PREDEF))),)
ARCH	:= i386
endif

ifneq ($(strip $(filter __i486__,$(PREDEF))),)
ARCH	:= i486
endif

ifneq ($(strip $(filter __i586__,$(PREDEF))),)
ARCH	:= i586
endif

ifneq ($(strip $(filter __i686__ ,$(PREDEF))),)
ARCH	:= i686
endif

ifneq ($(strip $(filter __amd64__ __amd64 __x86_64__ __x86_64, \
	$(PREDEF))),)
ARCH	:= x86_64
endif

ifneq ($(strip $(filter __arm__ __thumb__,$(PREDEF))),)

ifneq ($(strip $(filter __ARM_ARCH_6__,$(PREDEF))),)
ARCH	:= arm6
else
ARCH	:= arm
endif

endif

ifneq ($(strip $(filter __arm64__ ,$(PREDEF))),)
ARCH   := arm64
endif

ifneq ($(strip $(filter __mips__ __mips, $(PREDEF))),)
ARCH	:= mips
endif

ifneq ($(strip $(filter __powerpc __powerpc__ __POWERPC__ __ppc__ \
	_ARCH_PPC, $(PREDEF))),)
ARCH	:= ppc
endif

ifneq ($(strip $(filter __ppc64__ _ARCH_PPC64 , $(PREDEF))),)
ARCH	:= ppc64
endif

ifneq ($(strip $(filter __sparc__ __sparc __sparcv8 , $(PREDEF))),)

ifneq ($(strip $(filter __sparcv9 __sparc_v9__ , $(PREDEF))),)
ARCH	:= sparc64
else
ARCH	:= sparc
endif

endif

endif
endif


ifeq ($(ARCH),)
$(warning Could not detect ARCH)
endif


CFLAGS	+= -DARCH=\"$(ARCH)\"

ifeq ($(ARCH),mipsel)
CFLAGS += -march=mips32
endif


##############################################################################
#
# External libraries section
#

USE_OPENSSL := $(shell [ -f $(SYSROOT)/include/openssl/ssl.h ] || \
	[ -f $(SYSROOT)/local/include/openssl/ssl.h ] || \
	[ -f $(SYSROOT_ALT)/include/openssl/ssl.h ] && echo "yes")

ifneq ($(USE_OPENSSL),)
CFLAGS  += -DUSE_OPENSSL -DUSE_TLS
LIBS    += -lssl -lcrypto
USE_TLS := yes

USE_OPENSSL_DTLS := $(shell [ -f $(SYSROOT)/include/openssl/dtls1.h ] || \
	[ -f $(SYSROOT)/local/include/openssl/dtls1.h ] || \
	[ -f $(SYSROOT_ALT)/include/openssl/dtls1.h ] && echo "yes")

USE_OPENSSL_SRTP := $(shell [ -f $(SYSROOT)/include/openssl/srtp.h ] || \
	[ -f $(SYSROOT)/local/include/openssl/srtp.h ] || \
	[ -f $(SYSROOT_ALT)/include/openssl/srtp.h ] && echo "yes")

ifneq ($(USE_OPENSSL_DTLS),)
CFLAGS  += -DUSE_OPENSSL_DTLS -DUSE_DTLS
USE_DTLS := yes
endif

ifneq ($(USE_OPENSSL_SRTP),)
CFLAGS  += -DUSE_OPENSSL_SRTP -DUSE_DTLS_SRTP
USE_DTLS_SRTP := yes
endif

endif


USE_ZLIB    := $(shell [ -f $(SYSROOT)/include/zlib.h ] || \
	[ -f $(SYSROOT)/local/include/zlib.h ] || \
	[ -f $(SYSROOT_ALT)/include/zlib.h ] && echo "yes")

ifneq ($(USE_ZLIB),)
CFLAGS  += -DUSE_ZLIB
LIBS    += -lz
endif


ifneq ($(OS),win32)

HAVE_PTHREAD    := $(shell [ -f $(SYSROOT)/include/pthread.h ] && echo "1")
ifneq ($(HAVE_PTHREAD),)
HAVE_PTHREAD_RWLOCK := 1
CFLAGS  += -DHAVE_PTHREAD
HAVE_LIBPTHREAD := 1
ifneq ($(HAVE_LIBPTHREAD),)
LIBS	+= -lpthread
endif
endif

ifneq ($(ARCH),mipsel)
HAVE_GETIFADDRS := $(shell [ -f $(SYSROOT)/include/ifaddrs.h ] && echo "1")
ifneq ($(HAVE_GETIFADDRS),)
CFLAGS  += -DHAVE_GETIFADDRS
endif
endif

HAVE_STRERROR_R	:= 1
ifneq ($(HAVE_STRERROR_R),)
CFLAGS += -DHAVE_STRERROR_R
endif

endif

HAVE_GETOPT     := $(shell [ -f $(SYSROOT)/include/getopt.h ] && echo "1")
ifneq ($(HAVE_GETOPT),)
CFLAGS  += -DHAVE_GETOPT
endif
HAVE_INTTYPES_H := $(shell [ -f $(SYSROOT)/include/inttypes.h ] && echo "1")
ifneq ($(HAVE_INTTYPES_H),)
CFLAGS  += -DHAVE_INTTYPES_H
endif
HAVE_NET_ROUTE_H := $(shell [ -f $(SYSROOT)/include/net/route.h ] && echo "1")
ifneq ($(HAVE_NET_ROUTE_H),)
CFLAGS  += -DHAVE_NET_ROUTE_H
endif
HAVE_SYS_SYSCTL_H := \
	$(shell [ -f $(SYSROOT)/include/sys/sysctl.h ] || \
		[ -f $(SYSROOT)/include/$(MACHINE)/sys/sysctl.h ] \
		&& echo "1")
ifneq ($(HAVE_SYS_SYSCTL_H),)
CFLAGS  += -DHAVE_SYS_SYSCTL_H
endif

CFLAGS  += -DHAVE_STDBOOL_H

HAVE_INET6      := 1
ifneq ($(HAVE_INET6),)
CFLAGS  += -DHAVE_INET6
endif

ifeq ($(OS),win32)
CFLAGS  += -DHAVE_SELECT
CFLAGS  += -DHAVE_IO_H
else
HAVE_SYSLOG  := $(shell [ -f $(SYSROOT)/include/syslog.h ] && echo "1")
HAVE_DLFCN_H := $(shell [ -f $(SYSROOT)/include/dlfcn.h ] && echo "1")
ifneq ($(OS),darwin)
HAVE_EPOLL   := $(shell [ -f $(SYSROOT)/include/sys/epoll.h ] || \
			[ -f $(SYSROOT)/include/$(MACHINE)/sys/epoll.h ] \
			&& echo "1")
endif
ifneq ($(OS),openbsd)
HAVE_LIBRESOLV := $(shell [ -f $(SYSROOT)/include/resolv.h ] && echo "1")
endif

ifneq ($(HAVE_LIBRESOLV),)
CFLAGS  += -DHAVE_LIBRESOLV
ifneq ($(OS),freebsd)
LIBS    += -lresolv
endif
endif
ifneq ($(HAVE_SYSLOG),)
CFLAGS  += -DHAVE_SYSLOG
endif

HAVE_INET_NTOP := 1

CFLAGS  += -DHAVE_FORK

ifneq ($(HAVE_INET_NTOP),)
CFLAGS  += -DHAVE_INET_NTOP
endif
CFLAGS  += -DHAVE_PWD_H
ifneq ($(OS),darwin)
CFLAGS  += -DHAVE_POLL	# Darwin: poll() does not support devices
HAVE_INET_PTON := 1
endif
ifneq ($(HAVE_INET_PTON),)
CFLAGS  += -DHAVE_INET_PTON
endif
CFLAGS  += -DHAVE_SELECT -DHAVE_SELECT_H
CFLAGS  += -DHAVE_SETRLIMIT
CFLAGS  += -DHAVE_SIGNAL
CFLAGS  += -DHAVE_SYS_TIME_H
ifneq ($(HAVE_EPOLL),)
CFLAGS  += -DHAVE_EPOLL
endif
ifneq ($(HAVE_KQUEUE),)
CFLAGS  += -DHAVE_KQUEUE
endif
CFLAGS  += -DHAVE_UNAME
CFLAGS  += -DHAVE_UNISTD_H
ifneq ($(OS),cygwin)
CFLAGS  += -DHAVE_STRINGS_H
CFLAGS  += -DHAVE_GAI_STRERROR
endif
endif

ifneq ($(HAVE_ARC4RANDOM),)
CFLAGS  += -DHAVE_ARC4RANDOM
endif


##############################################################################
#
# Misc tools section
#
CCACHE	:= $(shell [ -e /usr/bin/ccache ] 2>/dev/null \
	|| [ -e /opt/local/bin/ccache ] \
	&& echo "ccache")
CFLAGS  += -DVERSION=\"$(VERSION)\"

# Enable gcov Coverage testing
#
# - generated during build: .gcno files
# - generated during exec:  .gcda files
#
ifneq ($(GCOV),)
CFLAGS += -fprofile-arcs -ftest-coverage
LFLAGS += -fprofile-arcs -ftest-coverage
# Disable ccache
CCACHE :=
endif

# gprof - GNU Profiling
#
# - generated during exec:  gmon.out
#
ifneq ($(GPROF),)
CFLAGS += -pg
LFLAGS += -pg
# Disable ccache
CCACHE :=
endif

CC	:= $(CCACHE) $(CC)
CFLAGS	+= $(EXTRA_CFLAGS)
LFLAGS	+= $(EXTRA_LFLAGS)

BUILD   := build-$(ARCH)


default:	all

.PHONY: distclean
distclean:
	@rm -rf build* *core*
	@rm -f *stamp $(BIN)
	@rm -f `find . -name "*.[oda]"` `find . -name "*.so"`
	@rm -f `find . -name "*~"` `find . -name "\.\#*"`
	@rm -f `find . -name "*.orig"` `find . -name "*.rej"`
	@rm -f `find . -name "*.previous"` `find . -name "*.gcov"`
	@rm -f `find . -name "*.exe"` `find . -name "*.dll"`
	@rm -f `find . -name "*.dylib"`

.PHONY: info
info:
	@echo "info - $(PROJECT) version $(VERSION)"
	@echo "  MODULES:       $(MODULES)"
#	@echo "  SRCS:          $(SRCS)"
	@echo "  MACHINE:       $(MACHINE)"
	@echo "  ARCH:          $(ARCH)"
	@echo "  OS:            $(OS)"
	@echo "  BUILD:         $(BUILD)"
	@echo "  CCACHE:        $(CCACHE)"
	@echo "  CC:            $(CC_NAME) $(CC_SHORTVER)"
	@echo "  CFLAGS:        $(CFLAGS)"
	@echo "  DFLAGS:        $(DFLAGS)"
	@echo "  LFLAGS:        $(LFLAGS)"
	@echo "  SH_LFLAGS:     $(SH_LFLAGS)"
	@echo "  MOD_LFLAGS:    $(MOD_LFLAGS)"
	@echo "  APP_LFLAGS:    $(APP_LFLAGS)"
	@echo "  LIBS:          $(LIBS)"
	@echo "  LIBRE_MK:      $(LIBRE_MK)"
	@echo "  LIBRE_INC:     $(LIBRE_INC)"
	@echo "  LIBRE_SO:      $(LIBRE_SO)"
	@echo "  USE_OPENSSL:   $(USE_OPENSSL)"
	@echo "  USE_TLS:       $(USE_TLS)"
	@echo "  USE_DTLS:      $(USE_DTLS)"
	@echo "  USE_DTLS_SRTP: $(USE_DTLS_SRTP)"
	@echo "  USE_ZLIB:      $(USE_ZLIB)"
	@echo "  GCOV:          $(GCOV)"
	@echo "  GPROF:         $(GPROF)"
	@echo "  CROSS_COMPILE: $(CROSS_COMPILE)"
	@echo "  SYSROOT:       $(SYSROOT)"
	@echo "  SYSROOT_ALT:   $(SYSROOT_ALT)"
	@echo "  LIB_SUFFIX:    $(LIB_SUFFIX)"
	@echo "  MOD_SUFFIX:    $(MOD_SUFFIX)"
	@echo "  BIN_SUFFIX:    $(BIN_SUFFIX)"


##############################################################################
#
# Packaging section
#
TAR_SRC   := $(PROJECT)-$(VERSION)

release:
	@rm -rf ../$(TAR_SRC)
	@svn export . ../$(TAR_SRC)
	@if [ -f ../$(TAR_SRC)/mk/exclude ]; then \
		cat ../$(TAR_SRC)/mk/exclude \
			| sed 's|^|../$(TAR_SRC)/|' | xargs -t rm -rf ; \
		rm -f ../$(TAR_SRC)/mk/exclude ; \
	fi
	@cd .. && rm -f $(TAR_SRC).tar.gz \
		&& tar -zcf $(TAR_SRC).tar.gz $(TAR_SRC) \
		&& echo "created release tarball `pwd`/$(TAR_SRC).tar.gz"

tar:
	@rm -rf ../$(TAR_SRC)
	@svn export . ../$(TAR_SRC)
	@cd .. && rm -f $(TAR_SRC).tar.gz \
		&& tar -zcf $(TAR_SRC).tar.gz $(TAR_SRC) \
		&& echo "created source tarball `pwd`/$(TAR_SRC).tar.gz"


# Debian
.PHONY: deb
deb:
	dpkg-buildpackage -rfakeroot

.PHONY: debclean
debclean:
	@rm -rf build-stamp configure-stamp debian/files debian/$(PROJECT) \
		debian/lib$(PROJECT) debian/lib$(PROJECT)-dev debian/tmp \
		debian/*.debhelper debian/*.debhelper.log debian/*.substvars

# RPM
RPM := $(shell [ -d /usr/src/rpm ] 2>/dev/null && echo "rpm")
ifeq ($(RPM),)
RPM := $(shell [ -d /usr/src/redhat ] 2>/dev/null && echo "redhat")
endif
.PHONY: rpm
rpm:    tar
	sudo cp ../$(PROJECT)-$(VERSION).tar.gz /usr/src/$(RPM)/SOURCES
	sudo rpmbuild -ba rpm/$(PROJECT).spec


##############################################################################
#
# Library and header files location section - in prioritised order
#
# - relative path
# - local installation
# - system installation
#

LIBRE_PATH := ../re

# Include path
LIBRE_INC := $(shell [ -f $(LIBRE_PATH)/include/re.h ] && \
	echo "$(LIBRE_PATH)/include")
ifeq ($(LIBRE_INC),)
LIBRE_INC := $(shell [ -f /usr/local/include/re/re.h ] && \
	echo "/usr/local/include/re")
endif
ifeq ($(LIBRE_INC),)
LIBRE_INC := $(shell [ -f /usr/include/re/re.h ] && echo "/usr/include/re")
endif

# Library path
LIBRE_SO  := $(shell [ -f $(LIBRE_PATH)/libre$(LIB_SUFFIX) ] \
	&& echo "$(LIBRE_PATH)")
ifeq ($(LIBRE_SO),)
LIBRE_SO  := $(shell [ -f /usr/local/lib/libre$(LIB_SUFFIX) ] \
	&& echo "/usr/local/lib")
endif
ifeq ($(LIBRE_SO),)
LIBRE_SO  := $(shell [ -f /usr/lib/libre$(LIB_SUFFIX) ] && echo "/usr/lib")
endif
ifeq ($(LIBRE_SO),)
LIBRE_SO  := $(shell [ -f /usr/lib64/libre$(LIB_SUFFIX) ] && echo "/usr/lib64")
endif


##############################################################################
#
# Splint section
#

SPLINT_OPTIONS += +unixlib
SPLINT_OPTIONS += +skipsysheaders
SPLINT_OPTIONS += -noeffect		# Statement has no effect: (void)reg
SPLINT_OPTIONS += -compdef		# Passed storage buf not completely..
SPLINT_OPTIONS += -mustfreefresh	# Fresh storage st not released before
SPLINT_OPTIONS += -nullret		# Null storage returned as non-null
SPLINT_OPTIONS += -compmempass
SPLINT_OPTIONS += -observertrans	# Observer storage returned without..
SPLINT_OPTIONS += -fixedformalarray     # e.g. param: int linesize[4]
SPLINT_OPTIONS += +voidabstract

# these options can be tuned:
SPLINT_OPTIONS += -mayaliasunique	# Parameter 1 (ua->stat.rphrase)
SPLINT_OPTIONS += -boolops		# Left operand of && is non-boolean
SPLINT_OPTIONS += -nullpass		# Possibly null storage resp passed..
SPLINT_OPTIONS += -type
SPLINT_OPTIONS += -mustfreeonly		# Implicitly only storage ua->callt..
SPLINT_OPTIONS += -nullassign
SPLINT_OPTIONS += -unrecog		# Unrecognized identifier: strdup
SPLINT_OPTIONS += -unqualifiedtrans	# Unqualified storage ua->lhost assign
SPLINT_OPTIONS += -temptrans		# Implicitly temp storage assigned to
SPLINT_OPTIONS += -usereleased
SPLINT_OPTIONS += -predboolint
SPLINT_OPTIONS += -statictrans
SPLINT_OPTIONS += -globstate		# Function returns with global..
SPLINT_OPTIONS += -compdestroy		# Only storage call->conf derived
SPLINT_OPTIONS += -onlytrans		# Only storage alias->mb assigned
SPLINT_OPTIONS += -shiftimplementation
SPLINT_OPTIONS += -shiftnegative
SPLINT_OPTIONS += -predboolothers
SPLINT_OPTIONS += -nullstate		# Null storage ct->timer_a derivable..
SPLINT_OPTIONS += -redef		# Enum member T1 defined more than once
SPLINT_OPTIONS += -usedef		# Variable tv used before definition
SPLINT_OPTIONS += -dependenttrans	# Dependent storage m returned as..
SPLINT_OPTIONS += -immediatetrans	# Immediate address &xyz returned
SPLINT_OPTIONS += -branchstate		# Storage is released in one path
SPLINT_OPTIONS += -kepttrans		# Kept storage le assigned to implic..
SPLINT_OPTIONS += -incondefs
SPLINT_OPTIONS += -exportlocal
SPLINT_OPTIONS += -nullderef
SPLINT_OPTIONS += -fullinitblock	# problem in main/main.c init block
SPLINT_OPTIONS += -evalorder		# problem with e.g. foo(rand(), rand())
SPLINT_OPTIONS += -uniondef		# union in struct sa
SPLINT_OPTIONS += -realcompare		# tping: comp double types rtt < 0
SPLINT_OPTIONS += -fcnuse
SPLINT_OPTIONS += -retvalother          # Return value ignored (not int)
SPLINT_OPTIONS += -Iinclude -I$(LIBRE_INC)
SPLINT_OPTIONS += -DHAVE_GETOPT -DHAVE_POLL -DHAVE_STDBOOL_H
SPLINT_OPTIONS += -DHAVE_INET_NTOP -DHAVE_INET_PTON -DHAVE_INET6

# ignore these files for now
SPLINT_IGNORE  := src/tls/openssl/tls.c src/tls/openssl/tls_tcp.c
SPLINT_IGNORE  += src/dns/darwin/srv.c src/aes/openssl/aes.c

SPLINT_SOURCES += $(filter-out $(SPLINT_IGNORE), $(patsubst %,src/%,$(SRCS)))


splint-sources:
	@echo $(SPLINT_SOURCES)

splint-all:
	@splint $(SPLINT_LIBS) $(SPLINT_OPTIONS) $(SPLINT_SOURCES)

splint:
	@for n in $(SPLINT_SOURCES); do \
		splint $(SPLINT_LIBS) $(SPLINT_OPTIONS) $${n} 2>/dev/null ; \
	done

splint-verbose:
	@for n in $(SPLINT_SOURCES); do \
		echo "running splint on $${n}"; \
		splint $(SPLINT_LIBS) $(SPLINT_OPTIONS) $${n} ; \
	done

splint-test:
	@splint $(SPLINT_LIBS) $(SPLINT_OPTIONS) test.c


###############################################################################
#
# Clang section
#

CLANG_OPTIONS := -Iinclude -I$(LIBRE_INC) $(CFLAGS)
CLANG_IGNORE  :=
CLANG_SRCS    += $(filter-out $(CLANG_IGNORE), $(patsubst %,src/%,$(SRCS)))

clang:
	@clang --analyze $(CLANG_OPTIONS) $(CLANG_SRCS)
	@rm -f *.plist


###############################################################################
#
# Documentation section
#
DOX_DIR=../$(PROJECT)-dox
DOX_TAR=$(PROJECT)-dox-$(VERSION)

$(DOX_DIR):
	@mkdir $@

$(DOX_DIR)/Doxyfile: mk/Doxyfile Makefile
	@cp $< $@
	@perl -pi -e 's/PROJECT_NUMBER\s*=.*/PROJECT_NUMBER = $(VERSION)/' \
	$(DOX_DIR)/Doxyfile

.PHONY:
dox:	$(DOX_DIR) $(DOX_DIR)/Doxyfile
	@doxygen $(DOX_DIR)/Doxyfile 2>&1 | grep -v DEBUG_ ; true
	@cd .. && rm -f $(DOX_TAR).tar.gz && \
	tar -zcf $(DOX_TAR).tar.gz $(PROJECT)-dox > /dev/null && \
	echo "Doxygen docs in `pwd`/$(DOX_TAR).tar.gz"
