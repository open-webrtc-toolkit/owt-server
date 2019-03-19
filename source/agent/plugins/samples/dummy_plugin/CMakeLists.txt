cmake_minimum_required (VERSION 2.8)

set (DUMMYPLUGIN_SRC myplugin.cc)
include_directories ("../../include")

ADD_DEFINITIONS (-fPIC -std=gnu++11)

add_library(myplugin SHARED ${DUMMYPLUGIN_SRC})

target_link_libraries(myplugin -ldl -fPIC -shared -std=gnu++11)
