
cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)

set(CUTTER_PYTHON_MIN 3.3)

option(CUTTER_ENABLE_PYTHON "Enable Python integration. Requires Python >= ${CUTTER_PYTHON_MIN}." ON)
option(CUTTER_ENABLE_PYTHON_BINDINGS "Enable generating Python bindings with Shiboken2. Unused if CUTTER_ENABLE_PYTHON=OFF." ON)
option(CUTTER_ENABLE_JUPYTER "Enable Jupyter integration. Unused if CUTTER_ENABLE_PYTHON=OFF." ON)
option(CUTTER_ENABLE_QTWEBENGINE "Use QtWebEngine for in-app Jupyter Browser. Unused if CUTTER_ENABLE_JUPYTER=OFF." OFF)


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

message(STATUS "version from Cutter.pro: ${CUTTER_VERSION_FULL}")
message(STATUS "sources from Cutter.pro: ${SOURCE_FILES}")
message(STATUS "headers from Cutter.pro: ${HEADER_FILES}")
message(STATUS "forms from Cutter.pro: ${UI_FILES}")
message(STATUS "resources from Cutter.pro: ${QRC_FILES}")

project(Cutter VERSION "${CUTTER_VERSION_FULL}")

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg Network)
if(CUTTER_ENABLE_JUPYTER AND CUTTER_ENABLE_QTWEBENGINE)
    find_package(Qt5 COMPONENTS WebEngineWidgets)
    if(NOT Qt5_FOUND)
        message(FATAL_ERROR "QtWebEngine could not be found which is required for the in-app Jupyter Browser.
If you do not want to enable this in-app Browser, re-run CMake with -DCUTTER_ENABLE_QTWEBENGINE=OFF.")
    endif()
    add_definitions(-DCUTTER_ENABLE_QTWEBENGINE)
endif()


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
    find_package(PythonLibs ${CUTTER_PYTHON_MIN})
    if(NOT PythonLibs_FOUND)
        message(FATAL_ERROR "Python >= ${CUTTER_PYTHON_MIN} could not be found.
If you do not want to build Cutter with Python, re-run CMake with -DCUTTER_ENABLE_PYTHON=OFF.")
    endif()

    include_directories(${PYTHON_INCLUDE_DIRS})
    add_definitions(-DCUTTER_ENABLE_PYTHON)

    if(CUTTER_ENABLE_JUPYTER)
        add_definitions(-DCUTTER_ENABLE_JUPYTER)
    endif()

    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        find_package(Shiboken2)
        find_package(PySide2)

        if((NOT Shiboken2_FOUND) OR (NOT PySide2_FOUND))
            message(FATAL_ERROR "Generating Python bindings is enabled, but Shiboken2 or PySide2 was not found.
If you explicitly want to not generate bindings, re-run CMake with -DCUTTER_ENABLE_PYTHON_BINDINGS=OFF
or, if you don't want to build with Python at all, use -DCUTTER_ENABLE_PYTHON=OFF.")
        endif()

        include_directories(${SHIBOKEN_INCLUDE_DIR})
        include_directories(${PYSIDE_INCLUDE_DIR})
        include_directories(${PYSIDE_INCLUDE_DIR}/QtCore)
    endif()
endif()


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
endif()


message(STATUS "")
message(STATUS "Building Cutter version ${CUTTER_VERSION_FULL}")
message(STATUS "Options:")
message(STATUS "- Python: ${CUTTER_ENABLE_PYTHON}")
message(STATUS "- Python Bindings: ${CUTTER_ENABLE_PYTHON_BINDINGS}")
message(STATUS "- Jupyter: ${CUTTER_ENABLE_JUPYTER}")
message(STATUS "- QtWebEngine: ${CUTTER_ENABLE_QTWEBENGINE}")
message(STATUS "")


include(QMakeConfigureFile)
qmake_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CutterConfig.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CutterConfig.h")


set(BINDINGS_SOURCE
        "${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings/cutterbindings_module_wrapper.cpp"
        #"${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings/cutterdockwidget_wrapper.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings/cuttercore_wrapper.cpp")

set_property(SOURCE ${BINDINGS_SOURCE} PROPERTY SKIP_AUTOGEN ON)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/widgets")

add_custom_command(OUTPUT ${BINDINGS_SOURCE}
        COMMAND shiboken2 --project-file=${CMAKE_CURRENT_BINARY_DIR}/bindings/bindings.txt
        #DEPENDS ${sample_TYPESYSTEM} ${CMAKE_CURRENT_SOURCE_DIR}/global.h shiboken2
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Python bindings with shiboken2")


add_executable(Cutter ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES} ${BINDINGS_SOURCE})

target_link_libraries(Cutter Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Svg Qt5::Network)
target_link_libraries(Cutter ${RADARE2_LIBRARIES})
if(CUTTER_ENABLE_PYTHON)
    target_link_libraries(Cutter ${PYTHON_LIBRARIES})
    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        target_link_libraries(Cutter ${SHIBOKEN_LIBRARY} ${PYSIDE_LIBRARY})
    endif()
endif()

if(CUTTER_ENABLE_PYTHON AND CUTTER_ENABLE_JUPYTER AND CUTTER_ENABLE_QTWEBENGINE)
    target_link_libraries(Cutter Qt5::WebEngineWidgets)
endif()


get_target_property(BINDINGS_INCLUDE_DIRS Cutter INCLUDE_DIRECTORIES)
list(APPEND BINDINGS_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}")
string(REPLACE ";" ":" BINDINGS_INCLUDE_DIRS "${BINDINGS_INCLUDE_DIRS}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bindings/bindings.txt.in"
        "${CMAKE_CURRENT_BINARY_DIR}/bindings/bindings.txt" @ONLY)
