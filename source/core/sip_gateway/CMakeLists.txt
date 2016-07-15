cmake_minimum_required(VERSION 2.8)
find_package(PkgConfig)
#functions
function(test_lib LIB_NAME)
  if (${LIB_NAME} MATCHES "^.*-NOTFOUND")
    message(FATAL_ERROR "lib not found: " ${LIB_NAME} " check README")
    return()
  endif(${LIB_NAME} MATCHES "^.*-NOTFOUND")
endfunction(test_lib)
project (SIP_GATEWAY)

set(SIP_GATEWAY_VERSION_MAJOR 0)
set(SIP_GATEWAY_VERSION_MINOR 1)
# TODO: Make a clean interface for the woogeen_base module so that we don't need
# to add such misc. ugly include paths.
set(LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libdeps/build/lib/")
set(LIBRE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/libre-0.4.16")
set(SIPUA_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/sipua")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${LIB_DIRS}../include/" "${CMAKE_CURRENT_SOURCE_DIR}/../woogeen_base/media")
include_directories("${LIBRE_DIRS}/include/" "${SIPUA_DIRS}/include/" )

file(GLOB SIP_GATEWAY_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
     "SipCallConnection.cpp"
     "SipGateway.cpp"
     "SipEP.cpp")
#     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(sip_gateway SHARED ${SIP_GATEWAY_SOURCES})

# Extra libs
find_library(RE re HINTS "${LIBRE_DIRS}")
test_lib(${RE})

find_library(SIPUA sipua HINTS "${SIPUA_DIRS}")
test_lib(${SIPUA})

target_link_libraries(sip_gateway ${EXTRA_LIBS} ${SIPUA} ${RE})
