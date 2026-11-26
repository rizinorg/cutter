# Test fixtures: binaries from rizin-testbins. Only the files listed here are fetched, through a
# shallow, blobless, sparse clone into test/bins. Each entry is
# <function in CutterTest::Fixtures> <path in rizin-testbins>.
# A path can also be a directory, ending in /. Fixtures.h is generated from this list.
set(CUTTER_TEST_FIXTURES
    helloX64    elf/analysis/hello-linux-x86_64
    crackmeX86  elf/ioli/crackme0x00
    coreutilsLs elf/ls
)

set(CUTTER_TESTBINS_DIR "" CACHE PATH
    "Path to a local clone of rizin-testbins. If set, test/bins is not cloned")

set(CUTTER_TEST_FIXTURE_FUNCTIONS "")
set(sparsePaths "")
list(LENGTH CUTTER_TEST_FIXTURES fixtureFieldCount)
math(EXPR lastFixtureField "${fixtureFieldCount} - 1")
foreach(nameIndex RANGE 0 ${lastFixtureField} 2)
    math(EXPR pathIndex "${nameIndex} + 1")
    list(GET CUTTER_TEST_FIXTURES ${nameIndex} fixtureName)
    list(GET CUTTER_TEST_FIXTURES ${pathIndex} fixturePath)
    string(APPEND CUTTER_TEST_FIXTURE_FUNCTIONS
        "\ninline QString ${fixtureName}()\n{\n    return QStringLiteral(\"${fixturePath}\");\n}\n")
    list(APPEND sparsePaths "/${fixturePath}")
endforeach()

string(STRIP "${CUTTER_TESTBINS_DIR}" CUTTER_TEST_FIXTURES_DIR)
if(NOT CUTTER_TEST_FIXTURES_DIR STREQUAL "")
    message(STATUS "Test fixtures: using your rizin-testbins clone at ${CUTTER_TEST_FIXTURES_DIR}")
else()
    set(CUTTER_TEST_FIXTURES_DIR "${CMAKE_SOURCE_DIR}/test/bins")
    find_package(Git QUIET)
    set(fixturesError "")
    if(NOT GIT_FOUND)
        set(fixturesError "git was not found")
    elseif(NOT EXISTS "${CUTTER_TEST_FIXTURES_DIR}/.git")
        message(STATUS "Test fixtures: cloning the files tests need from rizin-testbins into "
                       "${CUTTER_TEST_FIXTURES_DIR}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" clone --depth 1 --filter=blob:none --no-checkout
                    https://github.com/rizinorg/rizin-testbins.git "${CUTTER_TEST_FIXTURES_DIR}"
            RESULT_VARIABLE result ERROR_VARIABLE output)
        if(NOT result EQUAL 0)
            set(fixturesError "${output}")
            file(REMOVE_RECURSE "${CUTTER_TEST_FIXTURES_DIR}")
        endif()
    endif()
    # Run on every configure, so newly listed fixtures are fetched.
    if(fixturesError STREQUAL "")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" sparse-checkout set --no-cone ${sparsePaths}
            WORKING_DIRECTORY "${CUTTER_TEST_FIXTURES_DIR}"
            RESULT_VARIABLE result ERROR_VARIABLE output)
        if(result EQUAL 0)
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" checkout
                WORKING_DIRECTORY "${CUTTER_TEST_FIXTURES_DIR}"
                RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE output)
        endif()
        if(NOT result EQUAL 0)
            set(fixturesError "${output}")
        endif()
    endif()
    if(NOT fixturesError STREQUAL "")
        message(WARNING "Could not fetch the test fixtures (${fixturesError}). Tests that need "
                        "them will skip. If you have rizin-testbins cloned, set "
                        "CUTTER_TESTBINS_DIR=<path to it>")
    endif()
endif()

set(CUTTER_TEST_GENERATED_DIR "${CMAKE_BINARY_DIR}/test/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/Fixtures.h.in"
               "${CUTTER_TEST_GENERATED_DIR}/Fixtures.h" @ONLY)
