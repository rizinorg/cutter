message("Running windeployqt")
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
if(NOT WINDEPLOYQT_EXECUTABLE)
	message(FATAL_ERROR "Failed to find windeployqt")
endif()
execute_process(COMMAND "${WINDEPLOYQT_EXECUTABLE}" cutter.exe
        --plugindir "qtplugins"
        --no-translations # Cutter currently isn't loading Qt translation file
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
    RESULT_VARIABLE SCRIPT_RESULT)
if (SCRIPT_RESULT)
    message(FATAL_ERROR "Failed to bundle qt")
endif()
file(WRITE "${CMAKE_INSTALL_PREFIX}/qt.conf" "[PATHS]\nPlugins = qtplugins")
