
set(_module Shiboken2)

find_package(${_module} ${${_module}_FIND_VERSION} CONFIG)
set(_executable_target ${_module}::shiboken2)
set(_lib_target ${_module}::libshiboken)

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

if(TARGET ${_executable_target})
    get_target_property(_is_imported ${_executable_target} IMPORTED)
    if(_is_imported)
        get_target_property(_imported_location ${_executable_target} IMPORTED_LOCATION)
        if(NOT _imported_location)
            message(STATUS "Target ${_executable_target} does not specify its IMPORTED_LOCATION! Trying to find it ourselves...")
            find_file(SHIBOKEN_BINARY
                    shiboken2
                    PATHS "${SHIBOKEN_SHARED_LIBRARY_DIR}/../bin"
                    NO_DEFAULT_PATH)
            if(NOT SHIBOKEN_BINARY)
                set(_message_type WARNING)
                if(${_module}_FIND_REQUIRED)
                    set(_message_type FATAL_ERROR)
                endif()
                message(${_message_type} "Failed to manually find executable for ${_module}")
                return()
            endif()
            message(STATUS "IMPORTED_LOCATION for ${_executable_target} found: ${SHIBOKEN_BINARY}")
            set_target_properties(${_executable_target} PROPERTIES IMPORTED_LOCATION "${SHIBOKEN_BINARY}")
        endif()
    endif()

    get_target_property(_is_imported ${_lib_target} IMPORTED)
    if(_is_imported)
        get_target_property(_imported_location ${_lib_target} IMPORTED_LOCATION)
        if(NOT _imported_location)
            message(STATUS "Target ${_lib_target} does not specify its IMPORTED_LOCATION! Trying to find it ourselves...")
            find_library(SHIBOKEN_LIBRARY
                    NAMES
                    "shiboken2${SHIBOKEN_PYTHON_EXTENSION_SUFFIX}"
                    "shiboken2${SHIBOKEN_PYTHON_EXTENSION_SUFFIX}.${${_module}_FIND_VERSION_MAJOR}.${${_module}_FIND_VERSION_MINOR}"
                    PATHS "${SHIBOKEN_SHARED_LIBRARY_DIR}")
            if(NOT SHIBOKEN_LIBRARY)
                set(_message_type WARNING)
                if(${_module}_FIND_REQUIRED)
                    set(_message_type FATAL_ERROR)
                endif()
                message(${_message_type} "Failed to manually find library for ${_module}")
                return()
            endif()
            message(STATUS "IMPORTED_LOCATION for ${_lib_target} found: ${SHIBOKEN_LIBRARY}")
            set_target_properties(${_lib_target} PROPERTIES IMPORTED_LOCATION "${SHIBOKEN_LIBRARY}")
        endif()
    endif()
else()
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(${_module}
            FOUND_VAR ${_module}_FOUND
            REQUIRED_VARS SHIBOKEN_LIBRARY SHIBOKEN_INCLUDE_DIR SHIBOKEN_BINARY
            VERSION_VAR ${_module}_VERSION)

    add_library(${_lib_target} INTERFACE IMPORTED)
    set_target_properties(${_lib_target} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SHIBOKEN_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SHIBOKEN_LIBRARY}")

    add_executable(${_executable_target} IMPORTED)
    set_target_properties(${_executable_target} PROPERTIES
            IMPORTED_LOCATION "${SHIBOKEN_BINARY}")
endif()

mark_as_advanced(SHIBOKEN_INCLUDE_DIR SHIBOKEN_LIBRARY SHIBOKEN_BINARY)