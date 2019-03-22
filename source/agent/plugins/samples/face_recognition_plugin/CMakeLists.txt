# Copyright (C) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required (VERSION 2.8)
INCLUDE(GetPrerequisites)

set (CMAKE_LOCAL_DIR "${CMAKE_SOURCE_DIR}")

add_subdirectory(preprocess_tool)
add_subdirectory(server_plugin_simple)
add_subdirectory(server_plugin_classroom)
#add_subdirectory(server_plugin_body-face-style)
