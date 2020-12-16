include(QMakeConfigureFile)
qmake_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CutterConfig.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CutterConfig.h")

if(CUTTER_ENABLE_PYTHON_BINDINGS)
    set(BINDINGS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bindings")
    set(BINDINGS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/bindings")

    configure_file("${BINDINGS_SRC_DIR}/bindings.xml" "${BINDINGS_BUILD_DIR}/bindings.xml" COPYONLY) # trigger reconfigure if file changes

    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${BINDINGS_SRC_DIR}/src_list.py" cmake "${BINDINGS_BUILD_DIR}" OUTPUT_VARIABLE BINDINGS_SOURCE)

    set_property(SOURCE ${BINDINGS_SOURCE} PROPERTY SKIP_AUTOGEN ON)

    include_directories("${BINDINGS_BUILD_DIR}/CutterBindings")

    set(SHIBOKEN_OPTIONS)
    if (WIN32)
        list(APPEND SHIBOKEN_OPTIONS --avoid-protected-hack)
    endif()

    add_custom_command(OUTPUT ${BINDINGS_SOURCE}
            COMMAND Shiboken2::shiboken2 --project-file="${BINDINGS_BUILD_DIR}/bindings.txt" ${SHIBOKEN_OPTIONS} ${SHIBOKEN_EXTRA_OPTIONS}
            DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/bindings/bindings.xml" "${BINDINGS_BUILD_DIR}/bindings.txt"
            IMPLICIT_DEPENDS CXX "${CMAKE_CURRENT_SOURCE_DIR}/bindings/bindings.h"
            COMMENT "Generating Python bindings with shiboken2")
else()
    set(BINDINGS_SOURCE "")
endif()


if (TARGET Graphviz::GVC)
    list(APPEND SOURCE_FILES ${CUTTER_PRO_GRAPHVIZ_SOURCES})
    list(APPEND HEADER_FILES ${CUTTER_PRO_GRAPHVIZ_HEADERS})
endif()

if (WIN32)
    set(PLATFORM_RESOURCES "img/cutter.rc")
else()
    set(PLATFORM_RESOURCES "")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
    set_source_files_properties(${BINDINGS_SOURCE} PROPERTIES COMPILE_FLAGS -w)
endif()


add_executable(Cutter ${UI_FILES} ${QRC_FILES} ${PLATFORM_RESOURCES} ${SOURCE_FILES} ${HEADER_FILES} ${BINDINGS_SOURCE})
set_target_properties(Cutter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ..
        PDB_OUTPUT_DIRECTORY ..
        ENABLE_EXPORTS ON
        CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(Cutter PRIVATE CUTTER_SOURCE_BUILD)

set(CUTTER_INCLUDE_DIRECTORIES core widgets common plugins menus .)
foreach(_dir ${CUTTER_INCLUDE_DIRECTORIES})
    target_include_directories(Cutter PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${_dir}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cutter/${_dir}>
    )
endforeach()

if (TARGET Graphviz::GVC)
    target_link_libraries(Cutter PRIVATE Graphviz::GVC)
    target_compile_definitions(Cutter PRIVATE CUTTER_ENABLE_GRAPHVIZ)
endif()

if(CUTTER_ENABLE_CRASH_REPORTS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(Cutter PRIVATE Threads::Threads)

    add_definitions(-DCUTTER_ENABLE_CRASH_REPORTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
    if(DEFINED BREAKPAD_FRAMEWORK_DIR)
        include_directories("${BREAKPAD_FRAMEWORK_DIR}/Breakpad.framework/Headers")
        set_target_properties(Cutter PROPERTIES LINK_FLAGS "-Wl,-F${BREAKPAD_FRAMEWORK_DIR}")
        target_link_libraries(Cutter PRIVATE "-framework Breakpad")
    else()
        find_package(Breakpad REQUIRED)
        target_link_libraries(Cutter PRIVATE Breakpad::client)
    endif()
endif()

target_link_libraries(Cutter PUBLIC ${QT_PREFIX}::Core ${QT_PREFIX}::Widgets ${QT_PREFIX}::Gui PRIVATE  ${QT_PREFIX}::Svg ${QT_PREFIX}::Network)
target_link_libraries(Cutter PUBLIC ${RIZIN_TARGET})
if(CUTTER_ENABLE_PYTHON)
    if (WIN32)
        # On windows some of the Python STABLE API functions are in seperate library
        # which isn't added by CMake.
        foreach(_PYTHON_LIBRARY ${PYTHON_LIBRARIES})
            # Skip extra "optimized" and "debug" values that are only meant for passing to target_link_libraries()
            if((NOT _PYTHON_LIBRARY STREQUAL "optimized") AND (NOT _PYTHON_LIBRARY STREQUAL "debug"))
                get_filename_component(_PYTHON_LIB_DIR ${_PYTHON_LIBRARY} DIRECTORY)
                message(STATUS "Add extra library dir for Python: ${_PYTHON_LIB_DIR}")
                target_link_directories(Cutter PRIVATE ${_PYTHON_LIB_DIR})
            endif()
        endforeach()
    endif()
    target_link_libraries(Cutter PRIVATE ${PYTHON_LIBRARIES})
    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        target_link_libraries(Cutter PRIVATE Shiboken2::libshiboken PySide2::pyside2)

        get_target_property(RAW_BINDINGS_INCLUDE_DIRS Cutter INCLUDE_DIRECTORIES)
        set(BINDINGS_INCLUDE_DIRS "")
        foreach(_dir ${RAW_BINDINGS_INCLUDE_DIRS})
            string(REGEX REPLACE "\\$<BUILD_INTERFACE:(.*)>" "\\1" _dir ${_dir})
            string(REGEX REPLACE "\\$<INSTALL_INTERFACE:(.*)>" "" _dir ${_dir})
            if (NOT "${_dir}" STREQUAL "")
                list(APPEND BINDINGS_INCLUDE_DIRS "${_dir}")
            endif()
        endforeach()

        if(APPLE AND _qt5Core_install_prefix)
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include")
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtCore")
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtGui")
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtWidgets")
        endif()
        list(APPEND BINDINGS_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS})
        list(APPEND BINDINGS_INCLUDE_DIRS ${Rizin_INCLUDE_DIRS})
        list(APPEND BINDINGS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
        if (NOT WIN32)
            string(REPLACE ";" ":" BINDINGS_INCLUDE_DIRS "${BINDINGS_INCLUDE_DIRS}")
        endif()

        qmake_configure_file("${BINDINGS_SRC_DIR}/bindings.txt.in" "${BINDINGS_BUILD_DIR}/bindings.txt")
        add_compile_definitions(WIN32_LEAN_AND_MEAN)
    endif()
endif()

if(TARGET KF5::SyntaxHighlighting)
    target_link_libraries(Cutter PRIVATE KF5::SyntaxHighlighting)
    target_compile_definitions(Cutter PRIVATE CUTTER_ENABLE_KSYNTAXHIGHLIGHTING)
endif()

if (CUTTER_APPIMAGE_BUILD)
    target_compile_definitions(Cutter PRIVATE APPIMAGE)
endif()

if (CUTTER_PACKAGE_R2DEC)
    target_compile_definitions(Cutter PRIVATE CUTTER_APPVEYOR_R2DEC)
endif()

if(APPLE AND CUTTER_ENABLE_PACKAGING AND CUTTER_USE_BUNDLED_RIZIN)
	target_compile_definitions(Cutter PRIVATE MACOS_RZ_BUNDLED)
endif()

include(Translations)

# Install files
install(TARGETS Cutter
        EXPORT CutterTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        BUNDLE DESTINATION "." # needs to be tested
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Devel
)
install(EXPORT CutterTargets
    NAMESPACE Cutter::
    DESTINATION "${ConfigPackageLocation}"
    COMPONENT Devel)
install(FILES
    ../cmake/CutterConfig.cmake
    DESTINATION ${ConfigPackageLocation}
    COMPONENT Devel
)
foreach(_file ${HEADER_FILES})
    # Can't use target PUBLIC_HEADER option for installing due to multiple directories
    get_filename_component(_header_dir "${_file}" DIRECTORY)
    install (FILES "${_file}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cutter/${_header_dir}" COMPONENT Devel)
endforeach()

if(UNIX AND NOT APPLE)
    install (FILES "img/cutter.svg"
                DESTINATION "share/icons/hicolor/scalable/apps/")
    install(FILES "org.rizin.Cutter.desktop"
        DESTINATION "share/applications"
        COMPONENT Devel)
endif()
