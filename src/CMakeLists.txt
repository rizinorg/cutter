cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)
include(Utils)

set(CUTTER_PYTHON_MIN 3.5)

option(CUTTER_USE_BUNDLED_RADARE2 "Use radare2 from src/radare2 submodule instead of searching for it on the system" OFF)
option(CUTTER_USE_ADDITIONAL_RADARE2_PATHS "Search radare2 in additional paths which are not part of default system library paths.\
 Disable this option if you are linking against radare2 pacakged as proper system library or in a custom path and additional are paths causing problems." ON)
option(CUTTER_ENABLE_PYTHON "Enable Python integration. Requires Python >= ${CUTTER_PYTHON_MIN}." OFF)
option(CUTTER_ENABLE_PYTHON_BINDINGS "Enable generating Python bindings with Shiboken2. Unused if CUTTER_ENABLE_PYTHON=OFF." OFF)
option(CUTTER_ENABLE_CRASH_REPORTS "Enable crash report system. Unused if CUTTER_ENABLE_CRASH_REPORTS=OFF" OFF)
tri_option(CUTTER_ENABLE_KSYNTAXHIGHLIGHTING "Use KSyntaxHighlighting" AUTO)
tri_option(CUTTER_ENABLE_GRAPHVIZ "Enable use of graphviz for graph layout" AUTO)
option (option SHIBOKEN_EXTRA_OPTIONS "Extra options for shiboken generator")
set(CUTTER_EXTRA_PLUGIN_DIRS "" CACHE STRING "List of addition plugin locations")

if(NOT CUTTER_ENABLE_PYTHON)
    set(CUTTER_ENABLE_PYTHON_BINDINGS OFF)
endif()


# Parse Cutter.pro to get filenames
include(QMakeProParse)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Cutter.pro"
        "${CMAKE_CURRENT_BINARY_DIR}/Cutter.pro"
        COPYONLY) # trigger reconfigure if Cutter.pro changes
parse_qmake_pro("${CMAKE_CURRENT_BINARY_DIR}/Cutter.pro" CUTTER_PRO)
set(SOURCE_FILES ${CUTTER_PRO_SOURCES})
set(HEADER_FILES ${CUTTER_PRO_HEADERS})
set(UI_FILES ${CUTTER_PRO_FORMS})
set(QRC_FILES ${CUTTER_PRO_RESOURCES})
set(CUTTER_VERSION_MAJOR "${CUTTER_PRO_CUTTER_VERSION_MAJOR}")
set(CUTTER_VERSION_MINOR "${CUTTER_PRO_CUTTER_VERSION_MINOR}")
set(CUTTER_VERSION_PATCH "${CUTTER_PRO_CUTTER_VERSION_PATCH}")

set(CUTTER_VERSION_FULL "${CUTTER_VERSION_MAJOR}.${CUTTER_VERSION_MINOR}.${CUTTER_VERSION_PATCH}")

project(Cutter VERSION "${CUTTER_VERSION_FULL}")

set(CMAKE_CXX_STANDARD 11)


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg Network)

include(CutterInstallDirs)

if(CUTTER_USE_BUNDLED_RADARE2)
    include(BundledRadare2)
    set(RADARE2_TARGET Radare2)
else()
    find_package(Radare2 REQUIRED)
    set(RADARE2_TARGET Radare2::libr)
endif()

if(CUTTER_ENABLE_PYTHON)
    find_package(PythonLibs ${CUTTER_PYTHON_MIN} REQUIRED)

    include_directories(${PYTHON_INCLUDE_DIRS})
    add_definitions(-DCUTTER_ENABLE_PYTHON)

    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        find_package(PythonInterp REQUIRED)
        # 5.12.3 => 5.12
        if("${Qt5_VERSION}" MATCHES "^([0-9]+\\.[0-9]+)\\.[0-9]+")
            set(Shiboken2_VERSION_REQUIRED "${CMAKE_MATCH_1}")
        else()
            message(FATAL_ERROR "Failed to recognize Qt version")
        endif()
        find_package(Shiboken2 "${Shiboken2_VERSION_REQUIRED}" REQUIRED)
        find_package(PySide2 "${Shiboken2_VERSION_REQUIRED}" REQUIRED)

        get_target_property(PYSIDE_INCLUDE_DIR PySide2::pyside2 INTERFACE_INCLUDE_DIRECTORIES)
        list(GET PYSIDE_INCLUDE_DIR 0 PYSIDE_INCLUDE_DIR)
        include_directories(${PYSIDE_INCLUDE_DIR}
            ${PYSIDE_INCLUDE_DIR}/QtCore
            ${PYSIDE_INCLUDE_DIR}/QtGui
            ${PYSIDE_INCLUDE_DIR}/QtWidgets)

        add_definitions(-DCUTTER_ENABLE_PYTHON_BINDINGS)
    endif()
endif()

if(CUTTER_ENABLE_KSYNTAXHIGHLIGHTING)
    if(CUTTER_ENABLE_KSYNTAXHIGHLIGHTING STREQUAL AUTO)
        find_package(KF5SyntaxHighlighting)
        if(KF5SyntaxHighlighting_FOUND)
            set(KSYNTAXHIGHLIGHTING_STATUS ON)
        else()
            set(KSYNTAXHIGHLIGHTING_STATUS "OFF (KSyntaxHighlighting not found)")
        endif()
    else()
        find_package(KF5SyntaxHighlighting REQUIRED)
        set(KSYNTAXHIGHLIGHTING_STATUS ON)
    endif()
else()
    set(KSYNTAXHIGHLIGHTING_STATUS OFF)
endif()

if (CUTTER_ENABLE_GRAPHVIZ)
    if (CUTTER_ENABLE_GRAPHVIZ STREQUAL AUTO)
        find_package(Graphviz)
    else()
        find_package(Graphviz REQUIRED)
    endif()
    set (CUTTER_ENABLE_GRAPHVIZ ${Graphviz_FOUND})
endif()

message(STATUS "")
message(STATUS "Building Cutter version ${CUTTER_VERSION_FULL}")
message(STATUS "Options:")
message(STATUS "- Bundled radare2: ${CUTTER_USE_BUNDLED_RADARE2}")
message(STATUS "- Python: ${CUTTER_ENABLE_PYTHON}")
message(STATUS "- Python Bindings: ${CUTTER_ENABLE_PYTHON_BINDINGS}")
message(STATUS "- Crash Handling: ${CUTTER_ENABLE_CRASH_REPORTS}")
message(STATUS "- KSyntaxHighlighting: ${KSYNTAXHIGHLIGHTING_STATUS}")
message(STATUS "- Graphviz: ${CUTTER_ENABLE_GRAPHVIZ}")
message(STATUS "")


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

    add_custom_command(OUTPUT ${BINDINGS_SOURCE}
            COMMAND Shiboken2::shiboken2 --project-file="${BINDINGS_BUILD_DIR}/bindings.txt" ${SHIBOKEN_EXTRA_OPTIONS}
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


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
    set_source_files_properties(${BINDINGS_SOURCE} PROPERTIES COMPILE_FLAGS -w)
endif()


add_executable(Cutter MACOSX_BUNDLE ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES} ${BINDINGS_SOURCE})
set_target_properties(Cutter PROPERTIES
        ENABLE_EXPORTS ON
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist")
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
        include_directories(${BREAKPAD_INCLUDE_DIRS})
        target_link_libraries(Cutter PRIVATE ${BREAKPAD_LINK_LIBRARIES})
    endif()
endif()

target_link_libraries(Cutter PUBLIC Qt5::Core Qt5::Widgets Qt5::Gui PRIVATE  Qt5::Svg Qt5::Network)
target_link_libraries(Cutter PUBLIC ${RADARE2_TARGET})
if(CUTTER_ENABLE_PYTHON)
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
        list(APPEND BINDINGS_INCLUDE_DIRS ${Radare2_INCLUDE_DIRS})
        list(APPEND BINDINGS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
        string(REPLACE ";" ":" BINDINGS_INCLUDE_DIRS "${BINDINGS_INCLUDE_DIRS}")

        qmake_configure_file("${BINDINGS_SRC_DIR}/bindings.txt.in" "${BINDINGS_BUILD_DIR}/bindings.txt")
    endif()
endif()

if(TARGET KF5::SyntaxHighlighting)
    target_link_libraries(Cutter PRIVATE KF5::SyntaxHighlighting)
    target_compile_definitions(Cutter PRIVATE CUTTER_ENABLE_KSYNTAXHIGHLIGHTING)
endif()

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
    cmake/CutterConfig.cmake
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
    install(FILES "org.radare.Cutter.desktop"
        DESTINATION "share/applications"
        COMPONENT Devel)
endif()
