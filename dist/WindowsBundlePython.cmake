execute_process(COMMAND powershell "${CMAKE_CURRENT_LIST_DIR}/bundle_python.ps1" x64 "${CMAKE_INSTALL_PREFIX}"
    WORKING_DIRECTORY ${CPACK_PACKAGE_DIRECTORY}
    RESULT_VARIABLE SCRIPT_RESULT)
if (SCRIPT_RESULT)
    message(FATAL_ERROR "Failed to bundle python")
endif()
