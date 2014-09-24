AUTOMAKE_OPTIONS = foreign 1.11 no-dist-gzip dist-bzip2 subdir-objects
ACLOCAL_AMFLAGS = -I m4

INCLUDES = -I$(top_srcdir)/include -I. -I$(top_srcdir)/halloc
AM_CFLAGS = -ansi -pedantic -Wall -Wextra -Wno-long-long -O0 -g

SUBDIRS = docs

EXTRA_DIST = \
	AUTHORS README LICENSE \
	nestegg-uninstalled.pc.in \
	m4/as-ac-expand.m4 \
	m4/pkg.m4 \
	m4/ax_create_stdint_h.m4 \
	halloc/src/halloc.c \
	halloc/halloc.h \
	halloc/src/align.h \
	halloc/src/hlist.h \
	halloc/src/macros.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nestegg.pc

nesteggincludedir = $(includedir)/nestegg
nestegginclude_HEADERS = include/nestegg/nestegg.h include/nestegg/nestegg-stdint.h

lib_LTLIBRARIES = src/libnestegg.la

src_libnestegg_la_SOURCES = \
	src/nestegg.c \
	halloc/src/halloc.c \
	halloc/halloc.h \
	halloc/src/align.h \
	halloc/src/hlist.h \
	halloc/src/macros.h

check_PROGRAMS = test/test

test_test_SOURCES = test/test.c
test_test_LDADD = src/libnestegg.la

DISTCLEANFILES = include/nestegg/nestegg-stdint.h

dist-hook:
	find $(distdir) -type d -name '.git' | xargs rm -rf

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
