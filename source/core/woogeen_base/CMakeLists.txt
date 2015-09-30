cmake_minimum_required(VERSION 2.8)
find_package(PkgConfig)
#functions
function(test_lib LIB_NAME)
  if (${LIB_NAME} MATCHES "^.*-NOTFOUND")
    message(FATAL_ERROR "lib not found: " ${LIB_NAME} " check README")
    return()
  endif(${LIB_NAME} MATCHES "^.*-NOTFOUND")
endfunction(test_lib)
project (WOOGEEN_BASE)

set(WOOGEEN_BASE_VERSION_MAJOR 0)
set(WOOGEEN_BASE_VERSION_MINOR 1)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fPIC")

set(LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libdeps/build/lib/")
set(WEBRTC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/webrtc")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${LIB_DIRS}../include/" "${WEBRTC_DIRS}/src")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_LINUX)

file(GLOB_RECURSE WOOGEEN_BASE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/stream_synchronization.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_receiver.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_sync_module.cc")

add_library(woogeen_base STATIC ${WOOGEEN_BASE_SOURCES})

# WEBRTC
find_library(WEBRTC webrtc HINTS "${WEBRTC_DIRS}")
test_lib(${WEBRTC})

# libuv
find_library(LIBUV uv HINTS "${LIB_DIRS}")
test_lib(${LIBUV})

# LIBAV
find_library(AVCODEC avcodec HINTS "${LIB_DIRS}")
test_lib(${AVCODEC})

find_library(AVFORMAT avformat HINTS "${LIB_DIRS}")
test_lib(${AVFORMAT})
set (LIBAV_LIBS ${AVCODEC} ${AVFORMAT})

target_link_libraries(woogeen_base ${EXTRA_LIBS} ${WEBRTC} ${LIBUV} ${LIBAV_LIBS})
