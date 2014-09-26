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
set(CMAKE_CXX_FLAGS "-Wall -std=c++0x -fvisibility=hidden -fvisibility-inlines-hidden")

# TODO: Make a clean interface for the woogeen_base module so that we don't need
# to add such misc. ugly include paths.
set(LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libdeps/build/lib/")
set(UV_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/libuv-0.10.26")
set(WEBRTC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/webrtc")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${LIB_DIRS}../include/" "${UV_DIRS}/include/" "${WEBRTC_DIRS}/src")

file(GLOB_RECURSE MCU_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(mcu SHARED ${MCU_SOURCES})

# WEBRTC
find_library(WEBRTC webrtc HINTS "${WEBRTC_DIRS}")
test_lib(${WEBRTC})

# VPX
find_library(VPX vpx)
test_lib(${VPX})

target_link_libraries(mcu ${EXTRA_LIBS} ${WEBRTC} ${VPX})
