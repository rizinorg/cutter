# Test fixtures: binaries from rizin-testbins, downloaded at a pinned commit and checked by
# SHA-256, so the tests never need the full repository. Each entry is
# <function in CutterTest::Fixtures> <path in rizin-testbins> <sha256>.
# To add one, append an entry here; Fixtures.h is generated from this list.
set(CUTTER_TESTBINS_COMMIT f305b3dccf2f139290dd83f7a6e7cff663aa6b4f)
set(CUTTER_TEST_FIXTURES
    helloX64    elf/analysis/hello-linux-x86_64 7bdbf25324af1946ec0b16dbf928875a588a786f7c279cd115729c5a3a297a55
    crackmeX86  elf/ioli/crackme0x00            3aed9a3821134a2ab1d69cb455e5e9d80bb651a1c97af04cdba4f3bb0adaa37b
    coreutilsLs elf/ls                          15fa1384580e3bf7b1e2b3354cfb149e5eb5ea8b23cc29ed78166b67a2c70b00
)

option(CUTTER_FETCH_TEST_FIXTURES "Download the test fixtures from rizin-testbins" ON)
set(CUTTER_TESTBINS_DIR "" CACHE PATH
    "Path to a local clone of rizin-testbins; if set, the test fixtures are not downloaded")

function(cutter_fetch_test_fixture FIXTURE_PATH FIXTURE_HASH)
    set(destination "${CUTTER_TEST_FIXTURES_DIR}/${FIXTURE_PATH}")
    if(EXISTS "${destination}")
        file(SHA256 "${destination}" actual)
        if(actual STREQUAL FIXTURE_HASH)
            return()
        endif()
    endif()

    message(STATUS "Downloading test fixture ${FIXTURE_PATH}")
    set(url "https://raw.githubusercontent.com/rizinorg/rizin-testbins/${CUTTER_TESTBINS_COMMIT}/${FIXTURE_PATH}")
    file(DOWNLOAD "${url}" "${destination}" STATUS status TLS_VERIFY ON)
    list(GET status 0 code)
    if(NOT code EQUAL 0)
        list(GET status 1 reason)
        file(REMOVE "${destination}")
        message(WARNING "Could not download test fixture ${FIXTURE_PATH} (${reason}); tests using it will skip")
        return()
    endif()

    file(SHA256 "${destination}" actual)
    if(NOT actual STREQUAL FIXTURE_HASH)
        file(REMOVE "${destination}")
        message(WARNING "Test fixture ${FIXTURE_PATH} does not match its pinned SHA-256; tests using it will skip")
    endif()
endfunction()

string(STRIP "${CUTTER_TESTBINS_DIR}" CUTTER_TEST_FIXTURES_DIR)
set(fetchFixtures OFF)
set(useCheckout ON)
if(CUTTER_TEST_FIXTURES_DIR STREQUAL "")
    set(useCheckout OFF)
    set(CUTTER_TEST_FIXTURES_DIR "${CMAKE_BINARY_DIR}/test-fixtures")
    set(fetchFixtures ${CUTTER_FETCH_TEST_FIXTURES})
endif()

set(localCloneHint "If you already have rizin-testbins cloned, set CUTTER_TESTBINS_DIR=<path to it>")
if(useCheckout)
    message(STATUS "Test fixtures: using your local rizin-testbins clone at ${CUTTER_TEST_FIXTURES_DIR}")
elseif(fetchFixtures)
    message(STATUS "Test fixtures: downloading the files tests need from rizin-testbins "
                   "(commit ${CUTTER_TESTBINS_COMMIT}) into ${CUTTER_TEST_FIXTURES_DIR}. ${localCloneHint}")
else()
    message(STATUS "Test fixtures: download disabled; tests whose file is missing from "
                   "${CUTTER_TEST_FIXTURES_DIR} will be skipped. ${localCloneHint}")
endif()

set(CUTTER_TEST_FIXTURE_FUNCTIONS "")
list(LENGTH CUTTER_TEST_FIXTURES fixtureFieldCount)
math(EXPR lastFixtureField "${fixtureFieldCount} - 1")
foreach(nameIndex RANGE 0 ${lastFixtureField} 3)
    math(EXPR pathIndex "${nameIndex} + 1")
    math(EXPR hashIndex "${nameIndex} + 2")
    list(GET CUTTER_TEST_FIXTURES ${nameIndex} fixtureName)
    list(GET CUTTER_TEST_FIXTURES ${pathIndex} fixturePath)
    list(GET CUTTER_TEST_FIXTURES ${hashIndex} fixtureHash)

    string(APPEND CUTTER_TEST_FIXTURE_FUNCTIONS
        "\ninline QString ${fixtureName}()\n{\n    return QStringLiteral(\"${fixturePath}\");\n}\n")
    if(fetchFixtures)
        cutter_fetch_test_fixture(${fixturePath} ${fixtureHash})
    endif()
endforeach()

set(CUTTER_TEST_GENERATED_DIR "${CMAKE_BINARY_DIR}/test/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/Fixtures.h.in"
               "${CUTTER_TEST_GENERATED_DIR}/Fixtures.h" @ONLY)
