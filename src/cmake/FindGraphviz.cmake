set (_module Graphviz)

find_package(PkgConfig REQUIRED)
if (NOT (CMAKE_VERSION VERSION_LESS "3.12.0"))
    pkg_check_modules(GVC IMPORTED_TARGET GLOBAL libgvc)
elseif (NOT (CMAKE_VERSION VERSION_LESS "3.11.0"))
    pkg_check_modules(GVC IMPORTED_TARGET libgvc)
else()
    pkg_check_modules(GVC libgvc)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${_module}
        FOUND_VAR ${_module}_FOUND
        REQUIRED_VARS GVC_INCLUDE_DIRS)

if (${GVC_FOUND})
    if (CMAKE_VERSION VERSION_LESS "3.11.0")
        add_library(${_module}::GVC INTERFACE IMPORTED)
        set_target_properties(${_module}::GVC PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${GVC_INCLUDE_DIRS}")
        set_target_properties(${_module}::GVC PROPERTIES
                INTERFACE_LINK_LIBRARIES "${GVC_LIBRARIES}")
    else()
       add_library(${_module}::GVC ALIAS PkgConfig::GVC)
    endif()
endif()
