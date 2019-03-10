
set(_module Shiboken2)

find_package(${_module} ${${_module}_FIND_VERSION} CONFIG QUIET)

if(NOT ${_module}_FOUND)
    include(PythonInfo)
    find_python_site_packages(PYTHON_SITE_PACKAGES)
    get_python_extension_suffix(PYTHON_EXTENSION_SUFFIX)

    find_library(SHIBOKEN_LIBRARY
            NAMES
            "shiboken2${PYTHON_EXTENSION_SUFFIX}"
            "shiboken2${PYTHON_EXTENSION_SUFFIX}.${${_module}_FIND_VERSION_MAJOR}.${${_module}_FIND_VERSION_MINOR}"
            PATH_SUFFIXES "${PYTHON_SITE_PACKAGES}/shiboken2")

    find_path(SHIBOKEN_INCLUDE_DIR
            shiboken.h
            PATH_SUFFIXES "${PYTHON_SITE_PACKAGES}/shiboken2_generator/include")

    find_file(SHIBOKEN_BINARY
            shiboken2
            PATH_SUFFIXES "${PYTHON_SITE_PACKAGES}/shiboken2_generator")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${_module}
        FOUND_VAR ${_module}_FOUND
        REQUIRED_VARS SHIBOKEN_LIBRARY SHIBOKEN_INCLUDE_DIR SHIBOKEN_BINARY
        VERSION_VAR ${_module}_VERSION)

if(NOT TARGET ${_module}::shiboken2)
    add_library(${_module}::libshiboken INTERFACE IMPORTED)
    set_target_properties(${_module}::libshiboken PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SHIBOKEN_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SHIBOKEN_LIBRARY}")

    add_executable(${_module}::shiboken2 IMPORTED)
    set_target_properties(${_module}::shiboken2 PROPERTIES
            IMPORTED_LOCATION "${SHIBOKEN_BINARY}")
endif()

mark_as_advanced(SHIBOKEN_INCLUDE_DIR SHIBOKEN_LIBRARY SHIBOKEN_BINARY)