cmake_minimum_required(VERSION 2.8)

project (GATEWAY_ALL)

set(CMAKE_C_FLAGS "$ENV{CFLAGS} -Wall")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -Wall -std=c++11 -fvisibility-inlines-hidden -fno-deduce-init-list")

include_directories("common" "erizo/src/erizo")
add_subdirectory("woogeen_base")
