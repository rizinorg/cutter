
if (CUTTER_ENABLE_GRAPHVIZ STREQUAL AUTO)
    if (NOT (CMAKE_VERSION VERSION_LESS "3.6.0"))
        pkg_check_modules(GVC IMPORTED_TARGET libgvc)
    else()
        pkg_check_modules(GVC libgvc)
    endif()
    if (GVC_FOUND)
        set(CUTTER_ENABLE_GRAPHVIZ ON)
    else()
        set(CUTTER_ENABLE_GRAPHVIZ OFF)
    endif()
else()
    if (NOT (CMAKE_VERSION VERSION_LESS "3.6.0"))
        pkg_check_modules(GVC REQUIRED libgvc)
    else()
        pkg_check_modules(GVC REQUIRED libgvc)
    endif()
endif()

if (CMAKE_VERSION VERSION_LESS "3.6.0")
    add_library(PkgConfig::GVC INTERFACE IMPORTED)
    set_target_properties(PkgConfig::GVC PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GVC_INCLUDE_DIRS}")
    set_target_properties(PkgConfig::GVC PROPERTIES
            INTERFACE_LINK_LIBRARIES "${GVC_LIBRARIES}")
endif()

