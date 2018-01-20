
cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DisallowInSource)

project(cutter VERSION 1.1)

option(USE_SYSTEM_RADARE2 "Link against installed radare2")

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg)


if(WIN32)
    set(CUTTER_WIN32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cutter_win32")

    set(RADARE2_INCLUDE_DIRS
        "${CUTTER_WIN32_DIR}/radare2/include/libr"
        "${CUTTER_WIN32_DIR}/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        set(RADARE2_LIBRARY_DIRS "${CUTTER_WIN32_DIR}/radare2/lib64")
    else()
        set(RADARE2_LIBRARY_DIRS "${CUTTER_WIN32_DIR}/radare2/lib32")
    endif()

    set(RADARE2_LIBRARIES
        r_core
        r_config
        r_cons
        r_io
        r_util
        r_flag
        r_asm
        r_debug
        r_hash
        r_bin
        r_lang
        r_io
        r_anal
        r_parse
        r_bp
        r_egg
        r_reg
        r_search
        r_syscall
        r_socket
        r_fs
        r_magic
        r_crypto
        r_sdb)
endif()
if(UNIX)
    if(USE_SYSTEM_RADARE2)
        find_package(Radare2 REQUIRED)
    else()
        set(RADARE2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/../radare2/libr/include")
        set(RADARE2_LIBRARY_DIRS "${CMAKE_SOURCE_DIR}/../radare2/libr/")

        set(RADARE2_LIBRARIES
            r
            util
            pthread
            dl
            z)
    endif()
endif()

include_directories(${RADARE2_INCLUDE_DIRS})
link_directories(${RADARE2_LIBRARY_DIRS})

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
qt5_use_modules(cutter Core Widgets Gui Svg)
target_link_libraries(cutter ${RADARE2_LIBRARIES})
