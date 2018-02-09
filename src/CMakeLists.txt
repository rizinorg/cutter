
cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)

project(cutter VERSION 1.2)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg WebEngineWidgets)


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


find_package(PythonLibs 3 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
endif()


# Parse cutter.pro to get filenames
include(QMakeProParse)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cutter.pro"
        "${CMAKE_CURRENT_BINARY_DIR}/cutter.pro"
        COPYONLY) # trigger reconfigure if cutter.pro changes
parse_qmake_pro("${CMAKE_CURRENT_BINARY_DIR}/cutter.pro" CUTTER_PRO)
set(SOURCE_FILES ${CUTTER_PRO_SOURCES})
set(HEADER_FILES ${CUTTER_PRO_HEADERS})
set(UI_FILES ${CUTTER_PRO_FORMS})
set(QRC_FILES ${CUTTER_PRO_RESOURCES})

message(STATUS "sources from cutter.pro: ${SOURCE_FILES}")
message(STATUS "headers from cutter.pro: ${HEADER_FILES}")
message(STATUS "forms from cutter.pro: ${UI_FILES}")
message(STATUS "resources from cutter.pro: ${QRC_FILES}")


set(CUTTER_VERSION_SUFFIX "-dev")
set(CUTTER_VERSION_FULL "${PROJECT_VERSION}${CUTTER_VERSION_SUFFIX}")
message(STATUS "Building Cutter version ${CUTTER_VERSION_FULL}")
add_definitions("-DAPP_VERSION=\"${CUTTER_VERSION_FULL}\"")


add_executable(cutter ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES})
qt5_use_modules(cutter Core Widgets Gui Svg WebEngineWidgets)
target_link_libraries(cutter ${RADARE2_LIBRARIES})
target_link_libraries(cutter ${PYTHON_LIBRARIES})

