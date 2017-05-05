
cmake_minimum_required(VERSION 3.1)

project(Iaito VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui WebEngine WebEngineWidgets)


if(WIN32)
    # use radare2 libraries from submodule on windows
    set(IAITO_WIN32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iaito_win32")
    list(APPEND CMAKE_PREFIX_PATH "${IAITO_WIN32_DIR}")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        list(APPEND CMAKE_LIBRARY_PATH "${IAITO_WIN32_DIR}/radare2/lib64")
    else()
        list(APPEND CMAKE_LIBRARY_PATH "${IAITO_WIN32_DIR}/radare2/lib32")
    endif()

    set(RADARE2_INCLUDE_DIRS "${IAITO_WIN32_DIR}/radare2/include/libr" "${IAITO_WIN32_DIR}/include")
endif()


find_package(Radare2 REQUIRED)
include_directories(${RADARE2_INCLUDE_DIRS})
link_directories(${RADARE2_LIBRARY_DIRS})


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall)
endif()


# Parse iaito.pro to get filenames
include(QMakeProParse)
parse_qmake_pro("${CMAKE_CURRENT_SOURCE_DIR}/iaito.pro" IAITO_PRO)
set(SOURCE_FILES ${IAITO_PRO_SOURCES})
set(HEADER_FILES ${IAITO_PRO_HEADERS})
set(UI_FILES ${IAITO_PRO_FORMS})
set(QRC_FILES ${IAITO_PRO_RESOURCES})

message(STATUS "sources from iaito.pro: ${SOURCE_FILES}")
message(STATUS "headers from iaito.pro: ${HEADER_FILES}")
message(STATUS "forms from iaito.pro: ${UI_FILES}")
message(STATUS "resources from iaito.pro: ${QRC_FILES}")


set(IAITO_VERSION_SUFFIX "-dev")
set(IAITO_VERSION_FULL "${PROJECT_VERSION}${IAITO_VERSION_SUFFIX}")
message(STATUS "Building Iaito version ${IAITO_VERSION_FULL}")
add_definitions("-DAPP_VERSION=\"${IAITO_VERSION_FULL}\"")


add_executable(iaito ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES})
qt5_use_modules(iaito Core Widgets Gui WebEngine WebEngineWidgets)
target_link_libraries(iaito ${RADARE2_LIBRARIES})

