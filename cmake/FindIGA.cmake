# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: BSD-3-Clause

find_path(IGA_INCLUDE_DIR
  NAMES iga/iga.h)
find_library(IGA_LIBRARY
  NAMES iga64)

# As of IGC 2.1.12, the libiga64.so symlink is missing. This was reported but we're
# still awaiting a fix: https://github.com/intel/intel-graphics-compiler/issues/360
# Work around this by searching for the library by its SONAME (libigc64.so.2) where we
# found the header (in addition to IGA_ROOT). This is sort of an evil/fragile approach
# that probably will be _very_ problematic once IGC 3.x releases.
if(IGA_INCLUDE_DIR AND NOT IGA_LIBRARY)
  cmake_path(GET IGA_INCLUDE_DIR PARENT_PATH IGA_INCLUDE_ROOT)
  find_library(IGA_LIBRARY_2
    HINTS "${IGA_INCLUDE_ROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}" "${IGA_INCLUDE_ROOT}/lib"
    NAMES iga64 libiga64.so.2
    NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH)
  set(IGA_LIBRARY "${IGA_LIBRARY_2}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IGA
  FOUND_VAR IGA_FOUND
  REQUIRED_VARS
    IGA_INCLUDE_DIR
    IGA_LIBRARY
)

if(IGA_FOUND)
  add_library(IGA::iga UNKNOWN IMPORTED)
  set_target_properties(IGA::iga PROPERTIES
    IMPORTED_LOCATION "${IGA_LIBRARY}")
  target_include_directories(IGA::iga INTERFACE "${IGA_INCLUDE_DIR}")
endif()

mark_as_advanced(
  IGA_INCLUDE_DIR
  IGA_LIBRARY
)
