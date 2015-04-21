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
set(UV_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/libuv")
set(WEBRTC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/webrtc")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${LIB_DIRS}../include/" "${UV_DIRS}/include/" "${WEBRTC_DIRS}/src")

file(GLOB_RECURSE WOOGEEN_BASE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(woogeen_base STATIC ${WOOGEEN_BASE_SOURCES})

# WEBRTC
find_library(WEBRTC webrtc HINTS "${WEBRTC_DIRS}")
test_lib(${WEBRTC})

# libuv
find_library(LIBUV uv HINTS "${UV_DIRS}/")
test_lib(${LIBUV})

target_link_libraries(woogeen_base ${EXTRA_LIBS} ${WEBRTC} ${LIBUV})
