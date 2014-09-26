cmake_minimum_required(VERSION 2.8)

project (GATEWAY_ALL)
option (COMPILE_OOVOO_GATEWAY OFF)
option (COMPILE_MCU OFF)

include_directories("${GATEWAY_ALL_SOURCES}common")
add_subdirectory("${GATEWAY_ALL_SOURCES}erizo/src")

include_directories("${GATEWAY_ALL_SOURCES}common" "${GATEWAY_ALL_SOURCES}erizo/src/erizo")
add_subdirectory("${GATEWAY_ALL_SOURCES}woogeen_base")

if(COMPILE_OOVOO_GATEWAY)
  include_directories("${GATEWAY_ALL_SOURCES}common" "${GATEWAY_ALL_SOURCES}erizo/src/erizo" "${GATEWAY_ALL_SOURCES}woogeen_base")
  set (EXTRA_LIBS ${EXTRA_LIBS} erizo woogeen_base)
  add_subdirectory("${GATEWAY_ALL_SOURCES}oovoo_gateway")
endif(COMPILE_OOVOO_GATEWAY)

if(COMPILE_MCU)
  include_directories("${GATEWAY_ALL_SOURCES}common" "${GATEWAY_ALL_SOURCES}erizo/src/erizo" "${GATEWAY_ALL_SOURCES}woogeen_base")
  set (EXTRA_LIBS ${EXTRA_LIBS} erizo woogeen_base)
  add_subdirectory("${GATEWAY_ALL_SOURCES}mcu")
endif(COMPILE_MCU)
