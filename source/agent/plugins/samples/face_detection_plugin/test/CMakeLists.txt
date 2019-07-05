# Copyright (C) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "testFaceDetection")

set(CMAKE_SKIP_RPATH true)

find_package (OpenCV REQUIRED)

file (GLOB MAIN_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        )

include_directories ("..")
include_directories ("../../../include")

add_executable(${TARGET_NAME} ${MAIN_SRCS} ${MAIN_HEADERS})

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE"
COMPILE_PDB_NAME ${TARGET_NAME})

target_link_libraries(${TARGET_NAME} ${InferenceEngine_LIBRARIES} IE::ie_cpu_extension)
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBRARIES})
target_link_libraries(${TARGET_NAME} FaceDetectionPlugin)

