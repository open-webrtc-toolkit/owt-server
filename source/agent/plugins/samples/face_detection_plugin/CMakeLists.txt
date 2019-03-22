# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required (VERSION 2.8)

SET(OpenCV_STATIC OFF)
set(TARGET_NAME "facedetectionplugin")

set(CMAKE_SKIP_RPATH true)

set(CMAKE_LOCAL_DIR "${CMAKE_SOURCE_DIR}")

# Find OpenCV components if exist
find_package(OpenCV COMPONENTS highgui imgproc imgcodecs core QUIET)
if(NOT(OpenCV_FOUND))
    message(WARNING "OPENCV is disabled or not found, " ${TARGET_NAME} " skipped")
    return()
endif()

# Properties->C/C++->General->Additional Include Directories
include_directories (
    ${InferenceEngine_INCLUDE_DIRS}
)

file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        )

# Create named folders for the sources within the .vcproj
# Empty name lists them directly under the .vcproj
source_group("src" FILES ${MAIN_SRC})
source_group("include" FILES ${MAIN_HEADERS})

#Ana server side 
include_directories ("../../include")
include_directories ("../../")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib")
add_library(${TARGET_NAME} SHARED main.cpp ${MAIN_HEADERS})

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE"
COMPILE_PDB_NAME ${TARGET_NAME})
target_link_libraries(${TARGET_NAME} ${InferenceEngine_LIBRARIES} IE::ie_cpu_extension)
target_link_libraries(${TARGET_NAME} -ldl -fPIC -shared -std=gnu++11)

if(UNIX)
    target_link_libraries( ${TARGET_NAME} ${LIB_DL} pthread ${OpenCV_LIBRARIES} )
endif()
