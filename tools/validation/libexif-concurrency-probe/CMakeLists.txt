# Standalone libexif concurrency probe (FGC-0029). Deliberately does NOT link Qt:
# if this program corrupts the heap, the defect is in libexif (or how we call it),
# not in Qt or QGroundControl.
cmake_minimum_required(VERSION 3.24)
project(libexif_probe C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBEXIF_VERSION 0.6.26)
if(NOT DEFINED LIBEXIF_SOURCE_DIR)
    set(LIBEXIF_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.cache/CPM/libexif/4d0f")
endif()

if(NOT EXISTS "${LIBEXIF_SOURCE_DIR}/libexif/exif-data.c")
    message(FATAL_ERROR "libexif sources not found at ${LIBEXIF_SOURCE_DIR}")
endif()

add_library(exif STATIC
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-byte-order.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-content.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-data.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-entry.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-format.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-gps-ifd.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-ifd.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-loader.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-log.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-mem.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-mnote-data.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-tag.c
    ${LIBEXIF_SOURCE_DIR}/libexif/exif-utils.c
    ${LIBEXIF_SOURCE_DIR}/libexif/canon/exif-mnote-data-canon.c
    ${LIBEXIF_SOURCE_DIR}/libexif/canon/mnote-canon-entry.c
    ${LIBEXIF_SOURCE_DIR}/libexif/canon/mnote-canon-tag.c
    ${LIBEXIF_SOURCE_DIR}/libexif/fuji/exif-mnote-data-fuji.c
    ${LIBEXIF_SOURCE_DIR}/libexif/fuji/mnote-fuji-entry.c
    ${LIBEXIF_SOURCE_DIR}/libexif/fuji/mnote-fuji-tag.c
    ${LIBEXIF_SOURCE_DIR}/libexif/olympus/exif-mnote-data-olympus.c
    ${LIBEXIF_SOURCE_DIR}/libexif/olympus/mnote-olympus-entry.c
    ${LIBEXIF_SOURCE_DIR}/libexif/olympus/mnote-olympus-tag.c
    ${LIBEXIF_SOURCE_DIR}/libexif/pentax/exif-mnote-data-pentax.c
    ${LIBEXIF_SOURCE_DIR}/libexif/pentax/mnote-pentax-entry.c
    ${LIBEXIF_SOURCE_DIR}/libexif/pentax/mnote-pentax-tag.c
)
target_include_directories(exif PUBLIC ${LIBEXIF_SOURCE_DIR})
target_include_directories(exif PRIVATE ${LIBEXIF_SOURCE_DIR}/libexif ${CMAKE_CURRENT_BINARY_DIR}/Exif)
target_compile_definitions(exif PRIVATE HAVE_CONFIG_H)
set_target_properties(exif PROPERTIES C_STANDARD 99)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Exif/config.h" "
#pragma once
#define PACKAGE \"libexif\"
#define PACKAGE_VERSION \"${LIBEXIF_VERSION}\"
#define VERSION \"${LIBEXIF_VERSION}\"
#define GETTEXT_PACKAGE \"libexif-12\"
")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Exif/libexif")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Exif/libexif/_stdint.h" "
#pragma once
#include <stdint.h>
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif
")

target_include_directories(exif PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/Exif)

add_executable(exif_probe probe.cpp)
target_link_libraries(exif_probe PRIVATE exif)

# Keep a PDB next to the exe so a crash dump can be symbolized.
target_compile_options(exif_probe PRIVATE /Zi)
target_link_options(exif_probe PRIVATE /DEBUG /OPT:REF /OPT:ICF)
