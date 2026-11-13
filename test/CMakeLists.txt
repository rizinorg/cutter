find_package(Qt6 COMPONENTS Test REQUIRED)

set(TEST_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}
)

enable_testing()

include(${CMAKE_CURRENT_SOURCE_DIR}/common/Fixtures.cmake)

set(CUTTER_TEST_HARNESS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/TestEnv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/TestEnv.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/IsolatedSettings.h
    ${CUTTER_TEST_GENERATED_DIR}/Fixtures.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/UiHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/ViewHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/CallbackTask.h
)

function(cutter_add_layer_test LAYER TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp ${CUTTER_TEST_HARNESS_SOURCES})

    target_include_directories(${TEST_NAME} PRIVATE
        ${TEST_INCLUDES}
        ${CMAKE_SOURCE_DIR}/test/common
        ${CUTTER_TEST_GENERATED_DIR}
    )

    set_target_properties(${TEST_NAME} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    )

    target_link_libraries(${TEST_NAME} PRIVATE
        cutter_app
        Qt6::Test
        Qt6::Widgets
    )

    add_test(NAME ${LAYER}.${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)

    set_tests_properties(${LAYER}.${TEST_NAME} PROPERTIES
        LABELS "${LAYER}"
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;CUTTER_TESTBINS_DIR=${CUTTER_TEST_FIXTURES_DIR}"
        TIMEOUT 300
        SKIP_RETURN_CODE 77
    )
endfunction()

# Build targets for all tests (tests) and for one layer (tests_<layer>).
add_custom_target(tests)

function(cutter_add_layer_tests LAYER)
    add_custom_target(tests_${LAYER})
    add_dependencies(tests tests_${LAYER})
    foreach(TEST_NAME IN LISTS ARGN)
        cutter_add_layer_test(${LAYER} ${TEST_NAME})
        add_dependencies(tests_${LAYER} ${TEST_NAME})
    endforeach()
endfunction()

add_subdirectory(unit)
add_subdirectory(session)
add_subdirectory(dialogs)
add_subdirectory(workflows)
