
include(ExternalProject)

set(RADARE2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../radare2")
set(RADARE2_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Radare2-prefix")
set(MESON_OPTIONS "--prefix=${RADARE2_INSTALL_DIR}" --libdir=lib)

find_program(MESON meson)
if(NOT MESON)
    message(FATAL_ERROR "Failed to find meson, which is required to build bundled radare2")
endif()

find_program(NINJA ninja)
if(NOT NINJA)
    message(FATAL_ERROR "Failed to find ninja, which is required to build bundled radare2")
endif()

ExternalProject_Add(Radare2-Bundled
        SOURCE_DIR "${RADARE2_SOURCE_DIR}"
        INSTALL_DIR "${RADARE2_INSTALL_DIR}"
        CONFIGURE_COMMAND "${MESON}" "${RADARE2_SOURCE_DIR}" ${MESON_OPTIONS} && "${MESON}" configure ${MESON_OPTIONS}
        BUILD_COMMAND "${NINJA}"
        INSTALL_COMMAND "${NINJA}" install)

set(Radare2_INCLUDE_DIRS "${RADARE2_INSTALL_DIR}/include/libr")

add_library(Radare2 INTERFACE)
add_dependencies(Radare2 Radare2-Bundled)
if(NOT (${CMAKE_VERSION} VERSION_LESS "3.13.0"))
    target_link_directories(Radare2 INTERFACE "${RADARE2_INSTALL_DIR}/lib")
else()
    link_directories("${RADARE2_INSTALL_DIR}/lib")
endif()
target_link_libraries(Radare2 INTERFACE
        r_core r_config r_cons r_io r_util r_flag r_asm r_debug
        r_hash r_bin r_lang r_io r_anal r_parse r_bp r_egg r_reg
        r_search r_syscall r_socket r_fs r_magic r_crypto)
target_include_directories(Radare2 INTERFACE "${Radare2_INCLUDE_DIRS}")
