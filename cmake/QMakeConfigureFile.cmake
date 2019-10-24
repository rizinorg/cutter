# ------------------------
# QMakeConfigureFile.cmake
# ------------------------

function(_prepare_qmake_configure_file INPUT OUTPUT)
    # copyonly configure once to trigger re-running cmake on changes
    configure_file("${INPUT}" "${OUTPUT}.qmake.in" COPYONLY)
    file(READ "${INPUT}" CONTENT)

    # replace \" with "
    string(REPLACE "\\\"" "\"" CONTENT "${CONTENT}")
    # replace variables
    string(REGEX REPLACE "\\\$\\\$([A-Za-z0-9_]+)" "\${\\1}" CONTENT "${CONTENT}")
    string(REGEX REPLACE "\\\$\\\${([A-Za-z0-9_]+)}" "\${\\1}" CONTENT "${CONTENT}")

    file(WRITE "${OUTPUT}.cmake.in" "${CONTENT}")
endfunction()

# qmake_configure_file(<INPUT> <OUTPUT>)
#
# like configure_file, but using qmake syntax
#
macro(qmake_configure_file INPUT OUTPUT)
    _prepare_qmake_configure_file("${INPUT}" "${OUTPUT}")
    configure_file("${OUTPUT}.cmake.in" "${OUTPUT}")
endmacro()