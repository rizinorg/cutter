cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)
include(Utils)

set(CUTTER_PYTHON_MIN 3.5)

option(CUTTER_USE_BUNDLED_RIZIN "Use rizin from ./rizin submodule instead of searching for it on the system" ON)
option(CUTTER_USE_ADDITIONAL_RIZIN_PATHS "Search rizin in additional paths which are not part of default system library paths.\
 Disable this option if you are linking against rizin pacakged as proper system library or in a custom path and additional are paths causing problems." ON)
option(CUTTER_ENABLE_PYTHON "Enable Python integration. Requires Python >= ${CUTTER_PYTHON_MIN}." OFF)
option(CUTTER_ENABLE_PYTHON_BINDINGS "Enable generating Python bindings with Shiboken. Unused if CUTTER_ENABLE_PYTHON=OFF." OFF)
option(CUTTER_APPIMAGE_BUILD "Enable Appimage specific changes. Doesn't cause building of Appimage itself." OFF)
tri_option(CUTTER_ENABLE_KSYNTAXHIGHLIGHTING "Use KSyntaxHighlighting" AUTO)
tri_option(CUTTER_ENABLE_GRAPHVIZ "Enable use of graphviz for graph layout" AUTO)
set(SHIBOKEN_EXTRA_OPTIONS "" CACHE STRING "Extra options for shiboken generator")
set(CUTTER_EXTRA_PLUGIN_DIRS "" CACHE STRING "List of addition plugin locations")
option(CUTTER_ENABLE_DEPENDENCY_DOWNLOADS "Enable downloading of dependencies. Setting to OFF doesn't affect any downloads done by rizin build." OFF)
option(CUTTER_ENABLE_PACKAGING "Enable building platform-specific packages for distributing" OFF)
option(CUTTER_ENABLE_SIGDB "Downloads and installs sigdb (only available when CUTTER_USE_BUNDLED_RIZIN=ON)." OFF)
option(CUTTER_PACKAGE_DEPENDENCIES "During install step include the third party dependencies." OFF)
option(CUTTER_PACKAGE_RZ_GHIDRA "Compile and install rz-ghidra during install step." OFF)
option(CUTTER_PACKAGE_RZ_LIBSWIFT "Compile and install rz-libswift demangler during the install step." OFF)
option(CUTTER_PACKAGE_RZ_LIBYARA "Compile and install rz-libyara during the install step." OFF)
option(CUTTER_PACKAGE_RZ_SILHOUETTE "Compile and install rz-silhouette during the install step." OFF)
option(CUTTER_PACKAGE_JSDEC "Compile and install jsdec during install step." OFF)
OPTION(CUTTER_QT6 "Use QT6" OFF)

if(NOT CUTTER_ENABLE_PYTHON)
    set(CUTTER_ENABLE_PYTHON_BINDINGS OFF)
endif()

set(CUTTER_VERSION_MAJOR 2)
set(CUTTER_VERSION_MINOR 4)
set(CUTTER_VERSION_PATCH 0)

set(CUTTER_VERSION "${CUTTER_VERSION_MAJOR}.${CUTTER_VERSION_MINOR}.${CUTTER_VERSION_PATCH}")

execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip file
if ("${GIT_REV}" STREQUAL "")
    set(CUTTER_VERSION_FULL "${CUTTER_VERSION}")
else()
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH)
    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
    set(CUTTER_VERSION_FULL "${CUTTER_VERSION}-${GIT_BRANCH}-${GIT_REV}")
endif()

project(Cutter VERSION "${CUTTER_VERSION}")

# Enable solution folder support
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Put Qt files in a separate folder
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated Files")

set(CMAKE_CXX_STANDARD 11)

include(CutterInstallDirs)

if(CUTTER_USE_BUNDLED_RIZIN)
    include(BundledRizin)
    set(RIZIN_TARGET Rizin)
else()
    find_package(Rizin COMPONENTS Core REQUIRED)
    set(RIZIN_TARGET Rizin::Core)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(QT_COMPONENTS Core Widgets Gui Svg Network)
if (CUTTER_QT6)
    set(QT_PREFIX Qt6)
    list(APPEND QT_COMPONENTS Core5Compat SvgWidgets OpenGLWidgets)
else()
    set(QT_PREFIX Qt5)
endif()
find_package(${QT_PREFIX} REQUIRED COMPONENTS ${QT_COMPONENTS})

if(CUTTER_ENABLE_PYTHON)
    if(CMAKE_VERSION VERSION_LESS "3.12.0") 
        find_package(PythonInterp REQUIRED)
        find_package(PythonLibs ${CUTTER_PYTHON_MIN} REQUIRED)
        include_directories(${PYTHON_INCLUDE_DIRS})
    else()
        find_package (Python3 ${CUTTER_PYTHON_MIN} REQUIRED COMPONENTS Interpreter Development)
        set (PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    endif()

    add_definitions(-DCUTTER_ENABLE_PYTHON)

    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        if (CUTTER_QT6)
            # 6.12.3 => 6.12
            if("${Qt6_VERSION}" MATCHES "^([0-9]+\\.[0-9]+)\\.[0-9]+")
                set(Shiboken6_VERSION_REQUIRED "${CMAKE_MATCH_1}")
            else()
                message(FATAL_ERROR "Failed to recognize Qt version")
            endif()
            find_package(Shiboken6 "${Shiboken6_VERSION_REQUIRED}" REQUIRED)
            find_package(Shiboken6Tools "${Shiboken6_VERSION_REQUIRED}" REQUIRED)
            find_package(PySide6 "${Shiboken6_VERSION_REQUIRED}" REQUIRED)
            get_target_property(LIBSHIBOKEN_INCLUDE_DIRS Shiboken6::libshiboken INTERFACE_INCLUDE_DIRECTORIES)
            get_target_property(PYSIDE_INCLUDE_DIRS PySide6::pyside6 INTERFACE_INCLUDE_DIRECTORIES)
            # Check the presence of "pysidecleanup.h"
            include(CheckIncludeFileCXX)
            set(CMAKE_REQUIRED_INCLUDES "${PYSIDE_INCLUDE_DIRS};${LIBSHIBOKEN_INCLUDE_DIRS}")
            CHECK_INCLUDE_FILE_CXX("pysidecleanup.h" HAVE_PYSIDECLEANUP)
            add_compile_definitions("HAVE_PYSIDECLEANUP=${HAVE_PYSIDECLEANUP}")
        else()
            # 5.12.3 => 5.12
            if("${Qt5_VERSION}" MATCHES "^([0-9]+\\.[0-9]+)\\.[0-9]+")
                set(Shiboken2_VERSION_REQUIRED "${CMAKE_MATCH_1}")
            else()
                message(FATAL_ERROR "Failed to recognize Qt version")
            endif()
            find_package(Shiboken2 "${Shiboken2_VERSION_REQUIRED}" REQUIRED)
            find_package(PySide2 "${Shiboken2_VERSION_REQUIRED}" REQUIRED)
            get_target_property(PYSIDE_INCLUDE_DIRS PySide2::pyside2 INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        foreach(_dir IN LISTS PYSIDE_INCLUDE_DIRS)
            include_directories(${_dir}
                ${_dir}/QtCore
                ${_dir}/QtGui
                ${_dir}/QtWidgets)
        endforeach()

        add_definitions(-DCUTTER_ENABLE_PYTHON_BINDINGS)
    endif()
endif()

if(CUTTER_ENABLE_KSYNTAXHIGHLIGHTING AND (NOT CUTTER_QT6))
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
    if(CUTTER_ENABLE_KSYNTAXHIGHLIGHTING AND CUTTER_QT6)
        message(WARNING "KSyntaxHighlighting has been disabled because not supported in QT6")
    endif()
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
message(STATUS "- Bundled rizin: ${CUTTER_USE_BUNDLED_RIZIN}")
if(CUTTER_USE_BUNDLED_RIZIN)
    message(STATUS "- Bundled sigdb: ${CUTTER_ENABLE_SIGDB}")
endif()
message(STATUS "- Python: ${CUTTER_ENABLE_PYTHON}")
message(STATUS "- Python Bindings: ${CUTTER_ENABLE_PYTHON_BINDINGS}")
message(STATUS "- KSyntaxHighlighting: ${KSYNTAXHIGHLIGHTING_STATUS}")
message(STATUS "- Graphviz: ${CUTTER_ENABLE_GRAPHVIZ}")
message(STATUS "- Downloads dependencies: ${CUTTER_ENABLE_DEPENDENCY_DOWNLOADS}")
message(STATUS "- Enable Packaging: ${CUTTER_ENABLE_PACKAGING}")
message(STATUS "- Package Dependencies: ${CUTTER_PACKAGE_DEPENDENCIES}")
message(STATUS "- Package RzGhidra: ${CUTTER_PACKAGE_RZ_GHIDRA}")
message(STATUS "- Package RzLibSwift: ${CUTTER_PACKAGE_RZ_LIBSWIFT}")
message(STATUS "- Package RzLibYara: ${CUTTER_PACKAGE_RZ_LIBYARA}")
message(STATUS "- Package RzSilhouette: ${CUTTER_PACKAGE_RZ_SILHOUETTE}")
message(STATUS "- Package JSDec: ${CUTTER_PACKAGE_JSDEC}")
message(STATUS "- QT6: ${CUTTER_QT6}")
message(STATUS "")

add_subdirectory(src)

if(CUTTER_ENABLE_PACKAGING)
	add_subdirectory(dist)
endif()
