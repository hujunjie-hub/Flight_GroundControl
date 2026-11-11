# ============================================================================
# FGC-0029 ASan differential probe (diagnostic only - NOT part of the product build)
#
# Purpose: exercise the real production EXIF sources (ExifParser.cc / ExifUtility.cc)
# and the real libexif sources under MSVC AddressSanitizer with controlled thread
# counts, so that a heap error can be attributed to a specific stage:
#
#   a = QDateTime::fromString only            (Qt only)
#   b = libexif load/read/unref only          (libexif only)
#   c = ExifParser::readTime                  (libexif + Qt)
#   d = file read + ExifParser::readTime      (controller worker shape)
#   e = QtConcurrent::mapped + QFutureWatcher (controller async shape) + write path
#
# This project is intentionally separate from the QGroundControl build so that it can
# never influence the product binary. It builds its own ASan-instrumented libexif from
# the same pinned source tree that the product build uses.
# ============================================================================
cmake_minimum_required(VERSION 3.25)
project(fgc0029_asan_probe C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(FGC0029_ASAN "Build with MSVC AddressSanitizer" ON)

set(LIBEXIF_VERSION 0.6.26)
if(NOT DEFINED LIBEXIF_SOURCE_DIR)
    set(LIBEXIF_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.cache/CPM/libexif/4d0f")
endif()
if(NOT EXISTS "${LIBEXIF_SOURCE_DIR}/libexif/exif-data.c")
    message(FATAL_ERROR "libexif sources not found at ${LIBEXIF_SOURCE_DIR}")
endif()

if(NOT DEFINED QGC_SOURCE_DIR)
    get_filename_component(QGC_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
endif()

# ----------------------------------------------------------------------------
# libexif (same pinned sources as the product build), instrumented
# ----------------------------------------------------------------------------
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

# ----------------------------------------------------------------------------
# Qt
# ----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Gui Positioning)

# ----------------------------------------------------------------------------
# Probe executable: compiles the production sources unmodified
# ----------------------------------------------------------------------------
add_executable(fgc0029_asan_probe
    main.cpp
    exifparser_legacy.cpp
    spy_systemlocale.cpp
    "${QGC_SOURCE_DIR}/src/AnalyzeView/GeoTag/ExifParser.cc"
    "${QGC_SOURCE_DIR}/src/Utilities/Parsing/Exif/ExifUtility.cc"
)

target_include_directories(fgc0029_asan_probe PRIVATE
    "${Qt6Core_DIR}/../../../include/QtCore/6.11.1"
    "${Qt6Core_DIR}/../../../include/QtCore/6.11.1/QtCore"
    "${QGC_SOURCE_DIR}/src"
    "${QGC_SOURCE_DIR}/src/AnalyzeView/GeoTag"
    "${QGC_SOURCE_DIR}/src/Utilities/Parsing/Exif"
    "${QGC_SOURCE_DIR}/src/Utilities/Logging"
)

target_link_libraries(fgc0029_asan_probe PRIVATE Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::Positioning exif)

if(MSVC)
    # Keep optimisation on (RelWithDebInfo) so the race timing is not perturbed by -O0,
    # keep full debug info, and instrument every translation unit of this target.
    target_compile_options(fgc0029_asan_probe PRIVATE /Zi /W3 /utf-8 /EHsc)
    target_compile_options(exif PRIVATE /Zi)
    target_link_options(fgc0029_asan_probe PRIVATE /DEBUG)
    if(FGC0029_ASAN)
        target_compile_options(fgc0029_asan_probe PRIVATE /fsanitize=address)
        target_compile_options(exif PRIVATE /fsanitize=address)
        target_link_options(fgc0029_asan_probe PRIVATE /fsanitize=address)
        target_compile_definitions(fgc0029_asan_probe PRIVATE FGC0029_ASAN=1)
    endif()
endif()
