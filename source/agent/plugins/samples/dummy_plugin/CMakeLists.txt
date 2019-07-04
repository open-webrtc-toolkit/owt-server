# Copyright (C) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "DummyPlugin")

set (DUMMYPLUGIN_SRC myplugin.cc)
include_directories ("../../include")

add_library(${TARGET_NAME} SHARED ${DUMMYPLUGIN_SRC})

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE")

