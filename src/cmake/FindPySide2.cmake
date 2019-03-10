
set(_module PySide2)

find_package(${_module} ${${_module}_FIND_VERSION} CONFIG)

if(NOT ${_module}_FOUND)
    # TODO
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${_module}
        FOUND_VAR ${_module}_FOUND
        REQUIRED_VARS PYSIDE_LIBRARY PYSIDE_INCLUDE_DIR
        VERSION_VAR ${_module}_VERSION)

if(NOT TARGET ${_module}::pyside2)
    add_library(${_module}::pyside2 INTERFACE IMPORTED)
    set_target_properties(${_module}::pyside2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PYSIDE_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${PYSIDE_LIBRARY}")
endif()

mark_as_advanced(PYSIDE_INCLUDE_DIR PYSIDE_LIBRARY PYSIDE_BINARY)