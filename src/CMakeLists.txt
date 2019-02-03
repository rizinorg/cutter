
cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)

option(CUTTER_ENABLE_JUPYTER "Enable Jupyter integration. Requires Python >= 3.3." ON)
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

if(CUTTER_ENABLE_JUPYTER)
    find_package(PythonLibs 3.3)
    if(NOT PythonLibs_FOUND)
        message(FATAL_ERROR "Python >= 3.3 could not be found which is required for Jupyter integration.
If you do not want to enable Jupyter, re-run CMake with -DCUTTER_ENABLE_JUPYTER=OFF.")
    endif()

    include_directories(${PYTHON_INCLUDE_DIRS})
    add_definitions(-DCUTTER_ENABLE_JUPYTER)
endif()


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
endif()


message(STATUS "Building Cutter version ${CUTTER_VERSION_FULL}")


include(QMakeConfigureFile)
qmake_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CutterConfig.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CutterConfig.h")


find_package(Shiboken2 REQUIRED)
include_directories(${SHIBOKEN_INCLUDE_DIR})

find_package(PySide2 REQUIRED)
include_directories(${PYSIDE_INCLUDE_DIR})
include_directories(${PYSIDE_INCLUDE_DIR}/QtCore)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bindings/bindings.txt.in"
        "${CMAKE_CURRENT_BINARY_DIR}/bindings/bindings.txt" @ONLY)

set(BINDINGS_SOURCE
        "${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings/cutterbindings_module_wrapper.cpp"
        #"${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings/cutterdockwidget_wrapper.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings/cuttercore_wrapper.cpp")

set_property(SOURCE ${BINDINGS_SOURCE} PROPERTY SKIP_AUTOGEN ON)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/bindings/CutterBindings")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/widgets")

set(GENERATOR_EXTRA_FLAGS --generator-set=shiboken
        --enable-parent-ctor-heuristic
        --enable-pyside-extensions
        --enable-return-value-heuristic
        --use-isnull-as-nb_nonzero
        --typesystem-paths=${PYSIDE_TYPESYSTEMS})

add_custom_command(OUTPUT ${BINDINGS_SOURCE}
        COMMAND shiboken2 --project-file=${CMAKE_CURRENT_BINARY_DIR}/bindings/bindings.txt ${GENERATOR_EXTRA_FLAGS}
        #DEPENDS ${sample_TYPESYSTEM} ${CMAKE_CURRENT_SOURCE_DIR}/global.h shiboken2
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Python bindings with shiboken2")


add_executable(Cutter ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES} ${BINDINGS_SOURCE})

target_link_libraries(Cutter Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Svg Qt5::Network)
target_link_libraries(Cutter ${RADARE2_LIBRARIES})
if(CUTTER_ENABLE_JUPYTER)
    target_link_libraries(Cutter ${PYTHON_LIBRARIES})
    if(CUTTER_ENABLE_QTWEBENGINE)
        target_link_libraries(Cutter Qt5::WebEngineWidgets)
    endif()
endif()

target_link_libraries(Cutter ${SHIBOKEN_LIBRARY} ${PYSIDE_LIBRARY})

