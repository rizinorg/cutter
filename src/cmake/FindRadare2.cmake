# - Find Radare2 (libr)
#
#  RADARE2_FOUND          - True if libr has been found.
#  RADARE2_INCLUDE_DIRS   - libr include directory
#  RADARE2_LIBRARIES      - List of libraries when using libr.
#  RADARE2_LIBRARY_DIRS   - libr library directories

set(PKG_CONFIG_PATH
     $ENV{HOME}/bin/prefix/radare2/lib/pkgconfig
     /usr/local/lib/pkgconfig
     $ENV{PKG_CONFIG_PATH})
string(REPLACE ";" ":" PKG_CONFIG_PATH "${PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}")

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_search_module(RADARE2 r_core)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RADARE2 REQUIRED_VARS RADARE2_LIBRARIES RADARE2_INCLUDE_DIRS)
