
cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    cmake_policy(SET CMP0074 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)

set(CUTTER_PYTHON_MIN 3.5)

option(CUTTER_ENABLE_PYTHON "Enable Python integration. Requires Python >= ${CUTTER_PYTHON_MIN}." OFF)
option(CUTTER_ENABLE_PYTHON_BINDINGS "Enable generating Python bindings with Shiboken2. Unused if CUTTER_ENABLE_PYTHON=OFF." OFF)
option(CUTTER_ENABLE_CRASH_REPORTS "Enable crash report system. Unused if CUTTER_ENABLE_CRASH_REPORTS=OFF" OFF)

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

include_directories(core widgets common plugins)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg Network)


if(WIN32)
    # use radare2 libraries from submodule on windows
    set(CUTTER_WIN32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cutter_win32")
    list(APPEND CMAKE_PREFIX_PATH "${CUTTER_WIN32_DIR}")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        list(APPEND CMAKE_LIBRARY_PATH "${CUTTER_WIN32_DIR}/radare2/lib64")
    else()
        list(APPEND CMAKE_LIBRARY_PATH "${CUTTER_WIN32_DIR}/radare2/lib32")
    endif()

    set(RADARE2_INCLUDE_DIRS "${CUTTER_WIN32_DIR}/radare2/include/libr" "${CUTTER_WIN32_DIR}/include")
endif()


find_package(Radare2 REQUIRED)
include_directories(${RADARE2_INCLUDE_DIRS})
link_directories(${RADARE2_LIBRARY_DIRS})

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



message(STATUS "")
message(STATUS "Building Cutter version ${CUTTER_VERSION_FULL}")
message(STATUS "Options:")
message(STATUS "- Python: ${CUTTER_ENABLE_PYTHON}")
message(STATUS "- Python Bindings: ${CUTTER_ENABLE_PYTHON_BINDINGS}")
message(STATUS "- Crash Handling: ${CUTTER_ENABLE_CRASH_REPORTS}")
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
            COMMAND Shiboken2::shiboken2 --project-file="${BINDINGS_BUILD_DIR}/bindings.txt"
            DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/bindings/bindings.xml" "${BINDINGS_BUILD_DIR}/bindings.txt"
            IMPLICIT_DEPENDS CXX "${CMAKE_CURRENT_SOURCE_DIR}/bindings/bindings.h"
            COMMENT "Generating Python bindings with shiboken2")
else()
    set(BINDINGS_SOURCE "")
endif()



if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
    set_source_files_properties(${BINDINGS_SOURCE} PROPERTIES COMPILE_FLAGS -w)
endif()


add_executable(Cutter MACOSX_BUNDLE ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES} ${BINDINGS_SOURCE})
set_target_properties(Cutter PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist")

if(CUTTER_ENABLE_CRASH_REPORTS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(Cutter Threads::Threads)
    
    add_definitions(-DCUTTER_ENABLE_CRASH_REPORTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
    if(DEFINED BREAKPAD_FRAMEWORK_DIR)
        include_directories("${BREAKPAD_FRAMEWORK_DIR}/Breakpad.framework/Headers")
        set_target_properties(Cutter PROPERTIES LINK_FLAGS "-Wl,-F${BREAKPAD_FRAMEWORK_DIR}")
        target_link_libraries(Cutter "-framework Breakpad")
    else()
        find_package(Breakpad REQUIRED)
        include_directories(${BREAKPAD_INCLUDE_DIRS})
        target_link_libraries(Cutter ${BREAKPAD_LINK_LIBRARIES})
    endif()
endif()

target_link_libraries(Cutter Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Svg Qt5::Network)
target_link_libraries(Cutter ${RADARE2_LIBRARIES})
if(CUTTER_ENABLE_PYTHON)
    target_link_libraries(Cutter ${PYTHON_LIBRARIES})
    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        target_link_libraries(Cutter Shiboken2::libshiboken PySide2::pyside2)

        get_target_property(BINDINGS_INCLUDE_DIRS Cutter INCLUDE_DIRECTORIES)
        if(APPLE AND _qt5Core_install_prefix)
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include")
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtCore")
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtGui")
            list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtWidgets")
        endif()
        list(APPEND BINDINGS_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS})
        list(APPEND BINDINGS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
        string(REPLACE ";" ":" BINDINGS_INCLUDE_DIRS "${BINDINGS_INCLUDE_DIRS}")

        qmake_configure_file("${BINDINGS_SRC_DIR}/bindings.txt.in" "${BINDINGS_BUILD_DIR}/bindings.txt")
    endif()
endif()


