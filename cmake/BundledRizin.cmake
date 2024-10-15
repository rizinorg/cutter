
include(ExternalProject)

set(RIZIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rizin")
if(WIN32)
    set(RIZIN_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
        set(RIZIN_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
    endif()
    set(RIZIN_INSTALL_BINPATH ".")
    set(MESON_OPTIONS "--prefix=${CMAKE_INSTALL_PREFIX}" "--bindir=${RIZIN_INSTALL_BINPATH}")
else()
    set(RIZIN_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Rizin-prefix")
    set(MESON_OPTIONS "--prefix=${CMAKE_INSTALL_PREFIX}" --libdir=lib)
endif()

if (CUTTER_ENABLE_PACKAGING)
    list(APPEND MESON_OPTIONS "-Dportable=true")
endif()

if (CUTTER_ENABLE_SIGDB)
    list(APPEND MESON_OPTIONS "-Dinstall_sigdb=true")
endif()

if (CUTTER_PACKAGE_RZ_LIBSWIFT AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
    list(APPEND MESON_OPTIONS "-Duse_swift_demangler=false")
endif()

find_program(MESON meson)
if(NOT MESON)
    message(FATAL_ERROR "Failed to find meson, which is required to build bundled rizin")
endif()

find_program(NINJA ninja)
if(NOT NINJA)
    message(FATAL_ERROR "Failed to find ninja, which is required to build bundled rizin")
endif()

ExternalProject_Add(Rizin-Bundled
        SOURCE_DIR "${RIZIN_SOURCE_DIR}"
        CONFIGURE_COMMAND "${MESON}" "<SOURCE_DIR>" ${MESON_OPTIONS} && "${MESON}" configure ${MESON_OPTIONS} --buildtype "$<$<CONFIG:Debug>:debug>$<$<NOT:$<CONFIG:Debug>>:release>"
        BUILD_COMMAND "${NINJA}" && "DESTDIR=${RIZIN_INSTALL_DIR}" "${NINJA}" install
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND cmake -E echo "Skipping install step for Rizin-Bundled")

set(Rizin_INCLUDE_DIRS "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/include/librz" "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/include/librz/sdb")

add_library(Rizin INTERFACE)
add_dependencies(Rizin Rizin-Bundled)
if(NOT (${CMAKE_VERSION} VERSION_LESS "3.13.0"))
    target_link_directories(Rizin INTERFACE
        $<BUILD_INTERFACE:${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)
else()
    link_directories("${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/lib")
endif()

# TODO: This version number should be fetched automatically
# instead of being hardcoded.
set (Rizin_VERSION 0.8)

set (RZ_LIBS rz_core rz_config rz_cons rz_io rz_util rz_flag rz_arch rz_debug
        rz_hash rz_bin rz_lang rz_il rz_bp rz_egg rz_reg rz_search rz_syscall
        rz_socket rz_magic rz_crypto rz_type rz_diff rz_sign rz_demangler)
set (RZ_EXTRA_LIBS rz_main)
set (RZ_BIN rz-bin rizin rz-diff rz-find rz-gg rz-hash rz-run rz-asm rz-ax)

target_link_libraries(Rizin INTERFACE
        ${RZ_LIBS})
target_include_directories(Rizin INTERFACE
    "$<BUILD_INTERFACE:${Rizin_INCLUDE_DIRS}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/librz>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/librz/sdb>")

install(TARGETS Rizin EXPORT CutterTargets)
if (WIN32)
    foreach(_lib ${RZ_LIBS} ${RZ_EXTRA_LIBS})
        install(FILES "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/${_lib}-${Rizin_VERSION}.dll" DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endforeach()
    foreach(_exe ${RZ_BIN})
        install(FILES "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/${_exe}.exe" DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endforeach()
    install(DIRECTORY "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/share" DESTINATION ".")
    install(DIRECTORY "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/include" DESTINATION "."
        COMPONENT Devel)
    install(DIRECTORY "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/lib" DESTINATION "."
        COMPONENT Devel
        PATTERN "*.pdb" EXCLUDE)
else ()
    install(DIRECTORY "${RIZIN_INSTALL_DIR}/${CMAKE_INSTALL_PREFIX}/" DESTINATION "." USE_SOURCE_PERMISSIONS)
endif()
