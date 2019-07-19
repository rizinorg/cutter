
include(ExternalProject)

set(RADARE2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../radare2")
set(RADARE2_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Radare2-prefix")
set(MESON_OPTIONS "--prefix=${RADARE2_INSTALL_DIR}")

ExternalProject_Add(Radare2-Bundled
        SOURCE_DIR "${RADARE2_SOURCE_DIR}"
        INSTALL_DIR "${RADARE2_INSTALL_DIR}"
        CONFIGURE_COMMAND meson "${RADARE2_SOURCE_DIR}" ${MESON_OPTIONS} && meson configure ${MESON_OPTIONS}
        BUILD_COMMAND ninja
        INSTALL_COMMAND ninja install)

set(Radare2_INCLUDE_DIRS "${RADARE2_INSTALL_DIR}/include/libr")

add_library(Radare2 INTERFACE)
add_dependencies(Radare2 Radare2-Bundled)
target_link_directories(Radare2 INTERFACE "${RADARE2_INSTALL_DIR}/lib")
target_link_libraries(Radare2 INTERFACE
        r_core r_config r_cons r_io r_util r_flag r_asm r_debug
        r_hash r_bin r_lang r_io r_anal r_parse r_bp r_egg r_reg
        r_search r_syscall r_socket r_fs r_magic r_crypto)
target_include_directories(Radare2 INTERFACE "${Radare2_INCLUDE_DIRS}")
