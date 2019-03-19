 

cmake_minimum_required (VERSION 2.8)

set(TARGET_NAME "preprocess_tool")

set(CMAKE_SKIP_RPATH true)

if(OpenCV_FOUND)
    add_definitions(-DUSE_OPENCV)
endif()


# Properties->C/C++->General->Additional Include Directories
include_directories (
    ${InferenceEngine_INCLUDE_DIRS}
)


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


#Ana server side 
include_directories ("../../../include")
include_directories ("../../../")

#InferenceEngine side
include_directories("${INTEL_SAMPLE_DIR}/extension")
include_directories("${INTEL_SAMPLE_DIR}/extension/common")
include_directories("${INTEL_SAMPLE_DIR}/common/samples")
include_directories("../src")

#set(CMAKE_EXE_LINKER_FLAGS "-L${DL_SDK_LIB} ")
link_directories("/opt/intel/computer_vision_sdk/inference_engine/lib/ubuntu_16.04/intel64")

add_executable(${TARGET_NAME} main.cpp ${MAIN_HEADERS})


set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE"
COMPILE_PDB_NAME ${TARGET_NAME})


target_link_libraries(${TARGET_NAME}  cpu_extension inference_engine )
target_link_libraries(${TARGET_NAME} -ldl -fPIC -std=gnu++11)

if(UNIX)
    target_link_libraries( ${TARGET_NAME} ${LIB_DL} pthread ${OpenCV_LIBRARIES} )
endif()
