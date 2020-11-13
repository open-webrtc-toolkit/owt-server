# Copyright (C) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "CPUPipeline")

set (SAMPLEPIPELINE_SRC mypipeline.cc)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})

include_directories ("../../include")
include_directories ("../toml")

add_library(${TARGET_NAME} SHARED ${SAMPLEPIPELINE_SRC})

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE")

target_link_libraries(${TARGET_NAME} boost_thread)
target_link_libraries(${TARGET_NAME} gstreamer-1.0)
target_link_libraries(${TARGET_NAME} gobject-2.0)
target_link_libraries(${TARGET_NAME} glib-2.0)
target_link_libraries(${TARGET_NAME} gstapp-1.0)
target_link_libraries(${TARGET_NAME} gthread-2.0)
target_link_libraries(${TARGET_NAME} boost_system)

