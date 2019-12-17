# Copyright (C) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "SamplePipeline")

set (SAMPLEPIPELINE_SRC mypipeline.cc)
include_directories ("/usr/include/gstreamer-1.0")
include_directories ("/usr/include/glib-2.0")
include_directories ("/usr/lib/x86_64-linux-gnu/glib-2.0/include")
include_directories ("/home/owt-server/dist/analytics_agent/plugins/include")
include_directories ("../../include")

add_library(${TARGET_NAME} SHARED ${SAMPLEPIPELINE_SRC})

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE")

target_link_libraries(${TARGET_NAME} boost_thread)
target_link_libraries(${TARGET_NAME} gstreamer-1.0)
target_link_libraries(${TARGET_NAME} gobject-2.0)
target_link_libraries(${TARGET_NAME} glib-2.0)
target_link_libraries(${TARGET_NAME} gstapp-1.0)
target_link_libraries(${TARGET_NAME} gthread-2.0)
target_link_libraries(${TARGET_NAME} boost_system)

