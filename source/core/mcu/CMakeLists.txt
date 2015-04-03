cmake_minimum_required(VERSION 2.8)
find_package(PkgConfig)
#functions
function(test_lib LIB_NAME)
  if (${LIB_NAME} MATCHES "^.*-NOTFOUND")
    message(FATAL_ERROR "lib not found: " ${LIB_NAME} " check README")
    return()
  endif(${LIB_NAME} MATCHES "^.*-NOTFOUND")
endfunction(test_lib)
project (MCU)

set(MCU_VERSION_MAJOR 0)
set(MCU_VERSION_MINOR 1)

# TODO: Make a clean interface for the woogeen_base module so that we don't need
# to add such misc. ugly include paths.
set(LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libdeps/build/lib/")
set(UV_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/libuv")
set(WEBRTC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/webrtc")
set(H264_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/openh264")
set(MEDIAPROCESSOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/mediaprocessor")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${LIB_DIRS}../include/" "${UV_DIRS}/include/" "${WEBRTC_DIRS}/src")
include_directories("${MEDIAPROCESSOR_DIR}/libxcode/video/" "${MEDIAPROCESSOR_DIR}/libxcode/")

add_definitions(-DWEBRTC_POSIX -DWEBRTC_LINUX)

file(GLOB_RECURSE MCU_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/payload_router.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/stream_synchronization.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_encoder.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_receiver.cc"
    "${WEBRTC_DIRS}/src/webrtc/video_engine/vie_sync_module.cc")

add_library(mcu SHARED ${MCU_SOURCES})

# WEBRTC
find_library(WEBRTC webrtc HINTS "${WEBRTC_DIRS}")
test_lib(${WEBRTC})

#set(WEBRTC "-rdynamic -Wl,--whole-archive ${WEBRTC} -Wl,--no-whole-archive")

# VPX
find_library(VPX vpx)
test_lib(${VPX})

# H264
find_library(H264 openh264 HINTS "${H264_DIRS}")
test_lib(${H264})

set(VCSA_LIBS "")
if($ENV{BUILD_WITH_MSDK})
  find_library(XCODER_BASE xcodebase HINTS "${MEDIAPROCESSOR_DIR}/dist")
  test_lib(${XCODER_BASE})

  find_library(XCODER_VIDEO xcodevideo HINTS "${MEDIAPROCESSOR_DIR}/dist")
  test_lib(${XCODER_VIDEO})
  set(VCSA_LIBS ${XCODER_BASE} ${XCODER_VIDEO})
endif()

# LIBAV
find_library(AVCODEC avcodec HINTS "${LIB_DIRS}")
test_lib(${AVCODEC})

find_library(AVFORMAT avformat HINTS "${LIB_DIRS}")
test_lib(${AVFORMAT})
set (LIBAV_LIBS ${AVCODEC} ${AVFORMAT})

target_link_libraries(mcu ${EXTRA_LIBS} ${WEBRTC} ${VPX} ${H264} ${VCSA_LIBS} ${LIBAV_LIBS})
