
include(ExternalProject)

set(RADARE2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../radare2")
if(WIN32)
    set(RADARE2_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
        set(RADARE2_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
    endif()
    set(RADARE2_INSTALL_BINPATH ".")
    set(MESON_OPTIONS "--prefix=${RADARE2_INSTALL_DIR}" "--bindir=${RADARE2_INSTALL_BINPATH}" "-Dr2_incdir=include/libr")
else()
    set(RADARE2_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Radare2-prefix")
    set(MESON_OPTIONS "--prefix=${RADARE2_INSTALL_DIR}" --libdir=lib)
endif()

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
        CONFIGURE_COMMAND "${MESON}" "<SOURCE_DIR>" ${MESON_OPTIONS} && "${MESON}" configure ${MESON_OPTIONS}
        BUILD_COMMAND "${NINJA}"
        INSTALL_COMMAND "${NINJA}" install)

set(Radare2_INCLUDE_DIRS "${RADARE2_INSTALL_DIR}/include/libr")

add_library(Radare2 INTERFACE)
add_dependencies(Radare2 Radare2-Bundled)
if(NOT (${CMAKE_VERSION} VERSION_LESS "3.13.0"))
    target_link_directories(Radare2 INTERFACE
        $<BUILD_INTERFACE:${RADARE2_INSTALL_DIR}/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)
else()
    link_directories("${RADARE2_INSTALL_DIR}/lib")
endif()

set (R2_LIBS r_core r_config r_cons r_io r_util r_flag r_asm r_debug
        r_hash r_bin r_lang r_io r_anal r_parse r_bp r_egg r_reg
        r_search r_syscall r_socket r_fs r_magic r_crypto)
set (R2_EXTRA_LIBS r_main)
set (R2_BIN r2agent rabin2 radare2 radiff2 rafind2 ragg2 rahash2 rarun2 rasm2 rax2)

target_link_libraries(Radare2 INTERFACE
        ${R2_LIBS})
target_include_directories(Radare2 INTERFACE $<BUILD_INTERFACE:${Radare2_INCLUDE_DIRS}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libr>)

install(TARGETS Radare2 EXPORT CutterTargets)
if (APPLE)
elseif (WIN32)
    foreach(_lib ${R2_LIBS} ${R2_EXTRA_LIBS})
        install(FILES "${RADARE2_INSTALL_DIR}/${R2_INSTALL_BINPATH}/${_lib}.dll" DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endforeach()
    foreach(_exe ${R2_BIN})
        install(FILES "${RADARE2_INSTALL_DIR}/${R2_INSTALL_BINPATH}/${_exe}.exe" DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endforeach()
    install(DIRECTORY "${RADARE2_INSTALL_DIR}/share" DESTINATION ".")
    install(DIRECTORY "${RADARE2_INSTALL_DIR}/include" DESTINATION "."
        COMPONENT Devel)
    install(DIRECTORY "${RADARE2_INSTALL_DIR}/lib" DESTINATION "."
        COMPONENT Devel
        PATTERN "*.pdb" EXCLUDE)
else ()
    install(DIRECTORY "${RADARE2_INSTALL_DIR}/" DESTINATION ".")
endif()