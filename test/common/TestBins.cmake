# Test binaries from rizin-testbins. Only the files listed here are fetched into test/bins.
# Each entry is <function in CutterTest::Bins> <path in rizin-testbins>.
set(CUTTER_TEST_BINS
    helloX64    elf/analysis/hello-linux-x86_64
    crackmeX86  elf/ioli/crackme0x00
    coreutilsLs elf/ls
)

set(CUTTER_TEST_BIN_FUNCTIONS "")
set(sparsePaths "")
list(LENGTH CUTTER_TEST_BINS binFieldCount)
math(EXPR lastBinField "${binFieldCount} - 1")
foreach(nameIndex RANGE 0 ${lastBinField} 2)
    math(EXPR pathIndex "${nameIndex} + 1")
    list(GET CUTTER_TEST_BINS ${nameIndex} binName)
    list(GET CUTTER_TEST_BINS ${pathIndex} binFile)
    string(APPEND CUTTER_TEST_BIN_FUNCTIONS
        "\ninline QString ${binName}()\n{\n    return QStringLiteral(\"${binFile}\");\n}\n")
    list(APPEND sparsePaths "/${binFile}")
endforeach()

set(CUTTER_TESTBINS_DIR "${CMAKE_SOURCE_DIR}/test/bins" CACHE PATH "Path to rizin-testbins")

if(CUTTER_TESTBINS_DIR STREQUAL "${CMAKE_SOURCE_DIR}/test/bins")
    find_package(Git REQUIRED)
    if(NOT EXISTS "${CUTTER_TESTBINS_DIR}/.git")
        execute_process(COMMAND "${GIT_EXECUTABLE}" clone --depth 1 --filter=blob:none --no-checkout
                                https://github.com/rizinorg/rizin-testbins.git "${CUTTER_TESTBINS_DIR}")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" sparse-checkout set --no-cone ${sparsePaths}
                    WORKING_DIRECTORY "${CUTTER_TESTBINS_DIR}")
    execute_process(COMMAND "${GIT_EXECUTABLE}" checkout
                    WORKING_DIRECTORY "${CUTTER_TESTBINS_DIR}")
endif()

set(CUTTER_TEST_GENERATED_DIR "${CMAKE_BINARY_DIR}/test/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/TestBins.h.in"
               "${CUTTER_TEST_GENERATED_DIR}/TestBins.h" @ONLY)
