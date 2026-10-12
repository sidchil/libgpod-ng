cmake_minimum_required(VERSION 3.16)
project(libgpod VERSION 0.8.3 LANGUAGES C)

include(GNUInstallDirs)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckIncludeFile)

option(BUILD_SHARED_LIBS      "Build a shared library"                                  ON)
option(GPOD_ARTWORK           "Artwork / photo database support (needs gdk-pixbuf)"     ON)
option(GPOD_LIBXML            "Parse SysInfoExtended (needs libxml2)"                   ON)
option(GPOD_IMOBILEDEVICE     "iPhone / iPod touch support (needs libimobiledevice)"    OFF)
option(GPOD_BUILD_TESTS       "Build the self-contained tests"                          ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB   REQUIRED IMPORTED_TARGET glib-2.0>=2.32 gobject-2.0 gmodule-2.0)
pkg_check_modules(SQLITE REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(PLIST  IMPORTED_TARGET libplist-2.0)
if(NOT PLIST_FOUND)
  pkg_check_modules(PLIST REQUIRED IMPORTED_TARGET libplist>=1.0)
endif()
find_package(ZLIB)
if(ZLIB_FOUND)
  set(HAVE_ZLIB 1)
endif()

set(GPOD_PC_REQUIRES "glib-2.0 >= 2.32 gobject-2.0")
if(GPOD_ARTWORK)
  pkg_check_modules(PIXBUF REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0>=2.6.0)
  set(HAVE_GDKPIXBUF 1)
  string(APPEND GPOD_PC_REQUIRES " gdk-pixbuf-2.0")
endif()
if(GPOD_LIBXML)
  pkg_check_modules(LIBXML REQUIRED IMPORTED_TARGET libxml-2.0)
  set(HAVE_LIBXML 1)
endif()
if(GPOD_IMOBILEDEVICE)
  pkg_check_modules(IMOBILE REQUIRED IMPORTED_TARGET libimobiledevice-1.0>=0.9.7)
  set(HAVE_LIBIMOBILEDEVICE 1)
  pkg_check_modules(IMOBILE_115 QUIET libimobiledevice-1.0>=1.1.5)
  if(IMOBILE_115_FOUND)
    set(HAVE_LIBIMOBILEDEVICE_1_1_5 1)
  endif()
  string(APPEND GPOD_PC_REQUIRES " libimobiledevice-1.0")
endif()

check_include_file(unistd.h HAVE_UNISTD_H)
check_symbol_exists(localtime_r time.h HAVE_LOCALTIME_R)
check_struct_has_member("struct tm" tm_gmtoff time.h HAVE_STRUCT_TM_TM_GMTOFF)

# glib >= 2.32 always provides these (the sources carry fallbacks for ancient glib)
set(HAVE_G_INT64_HASH 1)
set(HAVE_G_INT64_EQUAL 1)
set(HAVE_G_CHECKSUM_RESET 1)
set(HAVE_G_MAPPED_FILE_UNREF 1)

set(GETTEXT_PACKAGE "libgpod")
set(LIBGPOD_BLOB_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/libgpod")
set(TMPMOUNTDIR "/tmp/")
configure_file(cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
# Make <gpod/itdb.h> resolvable inside the build tree exactly as it is once installed
configure_file(src/itdb.h ${CMAKE_CURRENT_BINARY_DIR}/include/gpod/itdb.h COPYONLY)

set(GPOD_SOURCES
  db-artwork-debug.c db-artwork-parser.c db-artwork-writer.c db-image-parser.c db-parse-context.c
  itdb_artwork.c itdb_chapterdata.c itdb_device.c itdb_hash58.c itdb_hash72.c itdb_hashAB.c
  itdb_iphone.c itdb_itunesdb.c itdb_photoalbum.c itdb_playlist.c itdb_plist.c itdb_sqlite.c
  itdb_sysinfo_extended_parser.c itdb_thumb.c itdb_track.c itdb_tzinfo.c itdb_zlib.c
  ithumb-writer.c pixmaps.c rijndael.c)
list(TRANSFORM GPOD_SOURCES PREPEND src/)

add_library(gpod ${GPOD_SOURCES})
add_library(libgpod::gpod ALIAS gpod)
set_target_properties(gpod PROPERTIES VERSION 4.3.2 SOVERSION 4 OUTPUT_NAME gpod)
target_compile_definitions(gpod PRIVATE HAVE_CONFIG_H LIBGPOD_BLOB_DIR="${LIBGPOD_BLOB_DIR}")
target_include_directories(gpod
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gpod-1.0>
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(gpod PUBLIC PkgConfig::GLIB PkgConfig::SQLITE PkgConfig::PLIST)
if(HAVE_ZLIB)
  target_link_libraries(gpod PRIVATE ZLIB::ZLIB)
endif()
if(HAVE_GDKPIXBUF)
  target_link_libraries(gpod PUBLIC PkgConfig::PIXBUF)
endif()
if(HAVE_LIBXML)
  target_link_libraries(gpod PRIVATE PkgConfig::LIBXML)
endif()
if(HAVE_LIBIMOBILEDEVICE)
  target_link_libraries(gpod PRIVATE PkgConfig::IMOBILE)
endif()
if(NOT WIN32)
  target_link_libraries(gpod PRIVATE m)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(gpod PRIVATE -Wall -Wno-deprecated-declarations)
endif()

# Install: library, header (<gpod/itdb.h>), pkg-config file, CMake package
install(TARGETS gpod EXPORT libgpodTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES src/itdb.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gpod-1.0/gpod)
configure_file(cmake/libgpod-1.0.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libgpod-1.0.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libgpod-1.0.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(EXPORT libgpodTargets NAMESPACE libgpod:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgpod)

if(GPOD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(cmake_tests)
endif()

message(STATUS "libgpod ${PROJECT_VERSION}: artwork=${GPOD_ARTWORK} libxml=${GPOD_LIBXML} imobiledevice=${GPOD_IMOBILEDEVICE}")
