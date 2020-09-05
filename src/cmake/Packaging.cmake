set(CUTTER_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../cutter-deps")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")

    if (CUTTER_PACKAGE_DEPENDENCIES)
        if (CUTTER_ENABLE_PYTHON)
            if (CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
                set(CPACK_INSTALL_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/WindowsBundlePython.cmake)
            endif()
            find_package(PythonInterp REQUIRED)
            install(DIRECTORY ${CUTTER_DEPS}/pyside/lib/site-packages DESTINATION "python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
            install(FILES ${CUTTER_DEPS}/pyside/bin/shiboken2.abi3.dll ${CUTTER_DEPS}/pyside/bin/pyside2.abi3.dll DESTINATION .)
        endif()
        install(SCRIPT cmake/WindowsBundleQt.cmake)

        if (CUTTER_PACKAGE_R2GHIDRA)
            if (CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
                # Currently using external project only for downloading source
                # It neeeds to link against compiled cutter so for now build it using custom install script.
                # That way r2ghidra build process is the same as with any other external plugin built against
                # installed Cutter.
                ExternalProject_Add(R2Ghidra
                    GIT_REPOSITORY https://github.com/radareorg/r2ghidra.git
                    GIT_TAG master
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND ""
                )
            endif()
            install(CODE "
                execute_process(
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/R2Ghidra-prefix/src/R2Ghidra-build
                    RESULT_VARIABLE SCRIPT_RESULT
                    COMMAND \${CMAKE_COMMAND} 
                                    ../R2Ghidra
                                    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=\"\${CMAKE_INSTALL_PREFIX}\;\${CMAKE_INSTALL_PREFIX}/include/libr\;\${CMAKE_INSTALL_PREFIX}/include/libr/sdb\"
                                    -DCMAKE_INSTALL_PREFIX=\${CMAKE_INSTALL_PREFIX}
                                    -DRADARE2_INSTALL_PLUGDIR=\${CMAKE_INSTALL_PREFIX}/lib/plugins
                                    -DCUTTER_INSTALL_PLUGDIR=plugins/native
                                    -DBUILD_CUTTER_PLUGIN=ON
                                    -DBUILD_SLEIGH_PLUGIN=OFF
                                    -G Ninja
                    )
                if (SCRIPT_RESULT)
                    message(FATAL_ERROR \"Failed to configure r2ghidra\")
                endif()
                execute_process(WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/R2Ghidra-prefix/src/R2Ghidra-build
                    RESULT_VARIABLE SCRIPT_RESULT
                    COMMAND \${CMAKE_COMMAND} --build . --target install
                    )
                if (SCRIPT_RESULT)
                    message(FATAL_ERROR \"Failed to install r2ghidra plugin\")
                endif()
            ")
        endif()
    endif()
endif()
include(CPack)
