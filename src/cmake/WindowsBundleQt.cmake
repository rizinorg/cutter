message("Running windeployqt")
execute_process(COMMAND windeployqt Cutter.exe
        --plugindir "qtplugins"
        --no-translations # Cutter currently isn't loading Qt translation file
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
    RESULT_VARIABLE SCRIPT_RESULT)
if (SCRIPT_RESULT)
    message(FATAL_ERROR "Failed to bundle python")
endif()
file(WRITE "${CMAKE_INSTALL_PREFIX}/qt.conf" "[PATHS]\nPlugins = qtplugins")
