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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libdeps/build/lib/")
set(WEBRTC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/webrtc")
set(H264_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/openh264")

pkg_check_modules(PC_YAMI libyami)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${LIB_DIRS}../include/" "${WEBRTC_DIRS}/src" "${PC_YAMI_INCLUDE_DIRS}")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_LINUX)

file(GLOB_RECURSE WOOGEEN_BASE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/stream_synchronization.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_receiver.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_sync_module.cc")

add_library(woogeen_base SHARED ${WOOGEEN_BASE_SOURCES})

# WEBRTC
find_library(WEBRTC webrtc HINTS "${WEBRTC_DIRS}")
test_lib(${WEBRTC})

# H264
find_library(H264 openh264 HINTS "${H264_DIRS}")
test_lib(${H264})

# LIBAV
find_library(AVCODEC avcodec HINTS "${LIB_DIRS}")
test_lib(${AVCODEC})

find_library(AVFORMAT avformat HINTS "${LIB_DIRS}")
test_lib(${AVFORMAT})
set (LIBAV_LIBS ${AVCODEC} ${AVFORMAT})

# BOOST
set (BOOST_LIBS thread)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(woogeen_base ${Boost_LIBRARIES})

# LOG4CXX
find_library(LOG log4cxx)
test_lib(${LOG})

find_library(YAMI yami HINTS ${PC_YAMI_LIBRARY_DIRS})

target_link_libraries(woogeen_base ${EXTRA_LIBS} ${WEBRTC} ${H264} ${LIBAV_LIBS} ${LOG} ${YAMI})
