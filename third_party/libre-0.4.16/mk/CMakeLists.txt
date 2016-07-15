cmake_minimum_required(VERSION 2.8.2)

project(re)
set(CMAKE_VERBOSE_MAKEFILE ON)

SET(RE_ROOT ${PROJECT_SOURCE_DIR}/..)
SET(RE_SRC_PREFIX ../src)

# include header files for the IDEs
file(GLOB_RECURSE HEADER_FILES src/*.h include/*.h)

if(UNIX)
	# get make db and extract information via regex's
	execute_process(COMMAND make --no-print-directory --just-print --print-data-base info OUTPUT_VARIABLE RE_MAKEDB OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY ${RE_ROOT})

	# get list of source files in SRCS = ...
	STRING(REGEX MATCH "[\n\r]SRCS = ([^\n\r]+)" RE_SRCS "${RE_MAKEDB}")
	STRING(REGEX REPLACE "[\n\r]SRCS = " "" RE_SRCS "${RE_SRCS}")
	# convert into list and prefix every element with 'src/'
	SET(RE_SRCS "${RE_SRC_PREFIX}/${RE_SRCS}")
	STRING(REPLACE " " ";${RE_SRC_PREFIX}/" RE_SRCS ${RE_SRCS})

	# get CFLAGS
	STRING(REGEX MATCH "[\n\r]CFLAGS = ([^\n\r]+)" RE_CFLAGS "${RE_MAKEDB}")
	STRING(REGEX REPLACE "[\n\r]CFLAGS = " "" RE_CFLAGS "${RE_CFLAGS}")
	# remove anything that is not a macro define
	STRING(REGEX REPLACE "-[^D][^ ]*" "" RE_CFLAGS "${RE_CFLAGS}" )

	# get EXTRA_CFLAGS
	STRING(REGEX MATCH "[\n\r]EXTRA_CFLAGS := ([^\n\r]+)" RE_EXTRA_CFLAGS "${RE_MAKEDB}")
	STRING(REGEX REPLACE "[\n\r]EXTRA_CFLAGS := " "" RE_EXTRA_CFLAGS "${RE_EXTRA_CFLAGS}")
	if (RE_EXTRA_CFLAGS)
		# not tested
		STRING(REGEX REPLACE "\\$\\(EXTRA_CFLAGS\\)" ${RE_EXTRA_CFLAGS} RE_CFLAGS "${RE_CFLAGS}")
	else()
		STRING(REGEX REPLACE "\\$\\(EXTRA_CFLAGS\\)" "" RE_CFLAGS "${RE_CFLAGS}")
	endif()

	# get OS
	STRING(REGEX MATCH "[\n\r]OS := ([^\n\r]+)" RE_OS "${RE_MAKEDB}")
	STRING(REGEX REPLACE "[\n\r]OS := " "" RE_OS "${RE_OS}")
	STRING(REGEX REPLACE "\\$\\(OS\\)" ${RE_OS} RE_CFLAGS "${RE_CFLAGS}")

	# get VERSION
	STRING(REGEX MATCH "[\n\r]VERSION := ([^\n\r]+)" RE_VERSION "${RE_MAKEDB}")
	STRING(REGEX REPLACE "[\n\r]VERSION := " "" RE_VERSION "${RE_VERSION}")
	STRING(REGEX REPLACE "\\$\\(VERSION\\)" ${RE_VERSION} RE_CFLAGS "${RE_CFLAGS}")

	# get ARCH
	STRING(REGEX MATCH "[\n\r]ARCH := ([^\n\r]+)" RE_ARCH "${RE_MAKEDB}")
	STRING(REGEX REPLACE "[\n\r]ARCH := " "" RE_ARCH "${RE_ARCH}")
	STRING(REGEX REPLACE "\\$\\(ARCH\\)" ${RE_ARCH} RE_CFLAGS "${RE_CFLAGS}")

	# escaping '\': makefiles do need it, but it breaks xcode - not sure who's "right"
	if (CMAKE_GENERATOR MATCHES Xcode)
		STRING(REGEX REPLACE "\\\\" "" RE_CFLAGS "${RE_CFLAGS}" )
	endif()

	if (APPLE)
		LIST(APPEND RE_SRCS "${RE_SRC_PREFIX}/lock/rwlock.c")
		
		# get MacOSX version
		execute_process(COMMAND /usr/bin/sw_vers -productVersion
			OUTPUT_VARIABLE MACOSX_VERSION
			ERROR_VARIABLE MACOSX_VERSION_errors
			RESULT_VARIABLE MACOSX_VERSION_result
		OUTPUT_STRIP_TRAILING_WHITESPACE)
		
		# build universal binaries
		set(CMAKE_OSX_ARCHITECTURES "x86_64;i386")
		
		if (${MACOSX_VERSION} VERSION_LESS 10.9)
		  foreach(FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
			    set(${FLAGS} "${${FLAGS}} -mmacosx-version-min=10.6 -stdlib=libstdc++")
		  endforeach()
		else()
		  foreach(FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
			    set(${FLAGS} "${${FLAGS}} -mmacosx-version-min=10.7 -stdlib=libc++")
		  endforeach()
		endif()
	endif()

	message("libre sources: ${RE_SRCS}")
	message("libre cflags: ${RE_CFLAGS}")
	message("libre extra_cflags: ${RE_EXTRA_CFLAGS}")
	message("libre OS: ${RE_OS}")
	message("libre arch: ${RE_ARCH}")
	message("libre version: ${RE_VERSION}")

elseif(WIN32)
	# hard-coded on Win32
	if(MSVC)
		SET(RE_CFLAGS
			"-DWIN32 -D_CONSOLE -D_CRT_SECURE_NO_DEPRECATE -DHAVE_SELECT -DHAVE_IO_H"
		)
	elseif(MINGW)
		SET(RE_CFLAGS
			"-DHAVE_STDBOOL_H -DHAVE_INET6 -DHAVE_SELECT -DHAVE_IO_H"
		)
		add_definitions(-Wall -D_WIN32_WINNT=0x0501)
	endif()
	# quotes get eaten in generator
	add_definitions(-DOS=\"win32\" -DWIN32 -DARCH=\"i386\" -DVERSION=\"0.3.0\")
	
	# on windows we cannot rely on make and have to do this by hand
	file(GLOB_RECURSE RE_SRCS RELATIVE ${PROJECT_SOURCE_DIR} ${RE_ROOT}/src/*.c)
	
	# remove files to compile depending on the compiler flags
	if (RE_CFLAGS MATCHES USE_ZLIB)
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/crc32/crc32.c")
	endif()

	if (NOT RE_CFLAGS MATCHES HAVE_PTHREAD_RWLOCK)
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/lock/rwlock.c")
	endif()

	if (NOT RE_CFLAGS MATCHES HAVE_GETIFADDRS)
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/net/ifaddrs.c")
	endif()
	
	if (NOT RE_CFLAGS MATCHES HAVE_EPOLL)
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/main/epoll.c")
	endif()
	
	if (NOT RE_CFLAGS MATCHES USE_OPENSSL)
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/tls/openssl/tls.c")
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/tls/openssl/tls_tcp.c")
	endif()
	
	if (NOT RE_CFLAGS MATCHES HAVE_PTHREAD)
		LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/lock/lock.c")
	endif()
	
	# remove files not to be comiled for win32 in any case
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/httpauth/basic.c")
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/mod/dl.c")
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/net/posix/pif.c")
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/net/linux/rt.c")
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/dns/res.c")
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/dns/darwin/srv.c")
	LIST(REMOVE_ITEM RE_SRCS "${RE_SRC_PREFIX}/net/bsd/brt.c")
	
endif()

include_directories(${RE_ROOT}/include)

add_library(re ${RE_SRCS} ${HEADER_FILES})

SET_TARGET_PROPERTIES(re PROPERTIES
	COMPILE_FLAGS ${RE_CFLAGS}
)
