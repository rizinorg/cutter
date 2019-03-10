
set(_module PySide2)

find_package(${_module} ${${_module}_FIND_VERSION} CONFIG QUIET)

if(NOT ${_module}_FOUND)
    include(PythonInfo)
    find_python_site_packages(PYTHON_SITE_PACKAGES)
    get_python_extension_suffix(PYTHON_EXTENSION_SUFFIX)

    find_library(PYSIDE_LIBRARY
            NAMES
                "pyside2${PYTHON_EXTENSION_SUFFIX}"
                "pyside2${PYTHON_EXTENSION_SUFFIX}.${${_module}_FIND_VERSION_MAJOR}.${${_module}_FIND_VERSION_MINOR}"
            PATH_SUFFIXES "${PYTHON_SITE_PACKAGES}/PySide2")

    find_path(PYSIDE_INCLUDE_DIR
            pyside.h
            PATH_SUFFIXES "${PYTHON_SITE_PACKAGES}/PySide2/include")

    find_path(PYSIDE_TYPESYSTEMS
            typesystem_core.xml
            PATH_SUFFIXES "${PYTHON_SITE_PACKAGES}/PySide2/typesystems")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${_module}
        FOUND_VAR ${_module}_FOUND
        REQUIRED_VARS PYSIDE_LIBRARY PYSIDE_INCLUDE_DIR PYSIDE_TYPESYSTEMS
        VERSION_VAR ${_module}_VERSION)

if(NOT TARGET ${_module}::pyside2)
    add_library(${_module}::pyside2 INTERFACE IMPORTED)
    set_target_properties(${_module}::pyside2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PYSIDE_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${PYSIDE_LIBRARY}")
endif()

mark_as_advanced(PYSIDE_INCLUDE_DIR PYSIDE_LIBRARY PYSIDE_BINARY)