cmake_minimum_required(VERSION 2.8)

project (GATEWAY_ALL)
option (COMPILE_OOVOO_GATEWAY OFF)
option (COMPILE_MCU OFF)

set(CMAKE_C_FLAGS "$ENV{CFLAGS} -Wall")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -Wall -std=c++0x -fvisibility-inlines-hidden -fno-deduce-init-list")

include_directories("common")
add_subdirectory("erizo/src")
include_directories("erizo/src/erizo")
add_subdirectory("woogeen_base")

if(COMPILE_OOVOO_GATEWAY)
  include_directories("woogeen_base")
  set (EXTRA_LIBS ${EXTRA_LIBS} erizo woogeen_base)
  add_subdirectory("oovoo_gateway")
endif(COMPILE_OOVOO_GATEWAY)

if(COMPILE_MCU)
  include_directories("woogeen_base")
  set (EXTRA_LIBS ${EXTRA_LIBS} woogeen_base)
  add_subdirectory("mcu")
endif(COMPILE_MCU)
