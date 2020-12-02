
set(_module PySide2)

find_package(${_module} ${${_module}_FIND_VERSION} CONFIG QUIET)
set(_lib_target ${_module}::pyside2)

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

if(TARGET ${_lib_target})
    get_target_property(_is_imported ${_lib_target} IMPORTED)
    if(_is_imported)
        get_target_property(_imported_location ${_lib_target} IMPORTED_LOCATION)
        if(NOT _imported_location)
            message(STATUS "Target ${_lib_target} does not specify its IMPORTED_LOCATION! Trying to find it ourselves...")
            set(_find_args)
            if(${_module}_CONFIG)
                get_filename_component(_pyside2_lib_dir "${${_module}_CONFIG}/../../../" ABSOLUTE)
                set(_find_args PATHS "${_pyside2_lib_dir}")
            endif()
            find_library(PYSIDE_LIBRARY
                    NAMES
                        "pyside2${PYTHON_CONFIG_SUFFIX}"
                        "pyside2${PYTHON_CONFIG_SUFFIX}.${${_module}_FIND_VERSION_MAJOR}.${${_module}_FIND_VERSION_MINOR}"
                    ${_find_args})
            if(NOT PYSIDE_LIBRARY)
                set(_message_type WARNING)
                if(${_module}_FIND_REQUIRED)
                    set(_message_type FATAL_ERROR)
                endif()
                message(${_message_type} "Failed to manually find library for ${_module}")
                return()
            endif()
            message(STATUS "IMPORTED_LOCATION for ${_lib_target} found: ${PYSIDE_LIBRARY}")
            set_target_properties(${_lib_target} PROPERTIES IMPORTED_LOCATION "${PYSIDE_LIBRARY}")
        endif()
    endif()
else()
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(${_module}
            FOUND_VAR ${_module}_FOUND
            REQUIRED_VARS PYSIDE_LIBRARY PYSIDE_INCLUDE_DIR PYSIDE_TYPESYSTEMS
            VERSION_VAR ${_module}_VERSION)

    add_library(${_module}::pyside2 INTERFACE IMPORTED)
    set_target_properties(${_module}::pyside2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PYSIDE_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${PYSIDE_LIBRARY}")
endif()

mark_as_advanced(PYSIDE_INCLUDE_DIR PYSIDE_LIBRARY PYSIDE_BINARY)