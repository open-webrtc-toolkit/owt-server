# Copyright (c) 2018 Intel Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


cmake_minimum_required (VERSION 2.8)
#INCLUDE(GetPrerequisites)

project(Samples)

SET(OpenCV_STATIC OFF)
set(TARGET_NAME "facedetectionplugin")

set(CMAKE_SKIP_RPATH true)

set(CMAKE_LOCAL_DIR "${CMAKE_SOURCE_DIR}")
set(ENV{PKG_CONFIG_PATH} "/opt/intel/computer_vision_sdk/opencv/lib/pkgconfig")

#hard-coded 
set (OUT_OF_TREE_DIR "/opt/intel/computer_vision_sdk/deployment_tools/inference_engine")
set (INTEL_SAMPLE_DIR "${OUT_OF_TREE_DIR}/samples")

list (APPEND CMAKE_MODULE_PATH ${INTEL_SAMPLE_DIR}/cmake)

include(CPUID)                 
include(OptimizationFlags)  

# environment variable InferenceEngine_DIR can be use instead of relative path to specify location of configuration file
get_filename_component(CMAKE_PREFIX_PATH "${OUT_OF_TREE_DIR}/share" ABSOLUTE)


find_package(InferenceEngine 1.0)

if (NOT InferenceEngine_FOUND)
    message(FATAL_ERROR "")
endif()

macro(enable_omp)
    if(UNIX) # Linux
        add_definitions(-fopenmp)
        find_library(intel_omp_lib iomp5
            PATHS ${InferenceEngine_INCLUDE_DIRS}/../external/mkltiny_lnx/lib
        )
    elseif(WIN32) # Windows
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
            set(OPENMP_FLAGS "/Qopenmp /openmp")
            set(CMAKE_SHARED_LINKER_FLAGS " ${CMAKE_SHARED_LINKER_FLAGS} /nodefaultlib:vcomp")
        elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
            set(OPENMP_FLAGS "/Qopenmp /openmp")
        else()
            message("Unknown compiler ID. OpenMP support is disabled.")
        endif()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPENMP_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENMP_FLAGS}")
        find_library(intel_omp_lib
            libiomp5md
            PATHS "${InferenceEngine_INCLUDE_DIRS}/../lib/intel64/${CMAKE_BUILD_TYPE}"
        )
    endif() endmacro(enable_omp) 
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "CMAKE_BUILD_TYPE not defined, 'Release' will be used")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (NOT(BIN_FOLDER))
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set (ARCH intel64)
    else()
        set (ARCH ia32)
    endif()

    set (BIN_FOLDER ${ARCH})
endif()


if (NOT (IE_MAIN_SOURCE_DIR))
    set(NEED_EXTENSIONS TRUE)
    if (WIN32)
        set (IE_MAIN_SOURCE_DIR ${CMAKE_LOCAL_DIR}/../bin/)
    else()
        set (IE_MAIN_SOURCE_DIR ${CMAKE_LOCAL_DIR}/build)
    endif()
endif()



if(NOT(UNIX))

    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (CMAKE_LIBRARY_PATH ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (CMAKE_PDB_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (LIBRARY_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER})
    set (LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_DIRECTORY}) # compatibility issue: linux uses LIBRARY_OUTPUT_PATH, windows uses LIBRARY_OUTPUT_DIRECTORY
else ()
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
    set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE})
    set (CMAKE_PDB_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE})
    set (LIBRARY_OUTPUT_DIRECTORY ${IE_MAIN_SOURCE_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
    set (LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_DIRECTORY}/lib)
endif()

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
if (WIN32)
    if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
        message(FATAL_ERROR "Only 64-bit supported on Windows")
    endif()

    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS -DNOMINMAX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc") #no asynchronous structured exception handling
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    if (ENABLE_OMP)
        find_package(OpenMP)
        if (OPENMP_FOUND)
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        endif()
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Werror=return-type ")
    if (APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-command-line-argument")
    elseif(UNIX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized -Winit-self -Wmaybe-uninitialized")
    endif()
endif()

#include(feature_defs OPTIONAL)

# Find OpenCV libray if exists
find_package(OpenCV 3.4 COMPONENTS core imgproc highgui imgcodecs)
include_directories(${OpenCV_INCLUDE_DIRS})
if(OpenCV_FOUND)
    add_definitions(-DUSE_OPENCV)
endif()

####################################
## to use C++11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
####################################


# Properties->C/C++->General->Additional Include Directories
include_directories (
    ${InferenceEngine_INCLUDE_DIRS}
)

if (UNIX)
    SET(LIB_DL dl)
endif()

if (NEED_EXTENSIONS)
    #add_subdirectory("${INTEL_SAMPLE_DIR}/extension"  "${INTEL_SAMPLE_DIR}/build/extension")
    add_subdirectory("${INTEL_SAMPLE_DIR}/extension"  "${CMAKE_LOCAL_DIR}/build/extension")
endif()



if(OpenCV_FOUND)
    message(STATUS "OPENCV is enabled")
else()
    message(STATUS "OPENCV is disabled or not found, " ${TARGET_NAME} " skiped\n")
    return()
endif()

if( BUILD_SAMPLE_NAME AND NOT ${BUILD_SAMPLE_NAME} STREQUAL ${TARGET_NAME} )
    message(STATUS "SAMPLE ${TARGET_NAME} SKIPPED")
    return()
endif()

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

include_directories (${OpenCV_INCLUDE_DIRS})
link_directories(${LIB_FOLDER})

#Ana server side 
include_directories ("../../include")
include_directories ("../../")

#InferenceEngine side
include_directories("${INTEL_SAMPLE_DIR}/extension")
include_directories("${INTEL_SAMPLE_DIR}/extension/common")
include_directories("${INTEL_SAMPLE_DIR}/common/samples")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../lib")

#set(CMAKE_EXE_LINKER_FLAGS "-L${DL_SDK_LIB} ")
link_directories("/opt/intel/computer_vision_sdk/inference_engine/lib/ubuntu_16.04/intel64")

add_library(${TARGET_NAME} SHARED main.cpp ${MAIN_HEADERS})

message(STATUS "IE_LIBRARIES=${IE_LIBRARIES}")
set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE"
COMPILE_PDB_NAME ${TARGET_NAME})
target_link_libraries(${TARGET_NAME} inference_engine ${IE_LIBRARIES})
target_link_libraries(${TARGET_NAME} -ldl -fPIC -shared -std=gnu++11)

if(UNIX)
    target_link_libraries( ${TARGET_NAME} ${LIB_DL} pthread ${OpenCV_LIBRARIES} )
endif()
