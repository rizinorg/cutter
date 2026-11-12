# helper function to register a single test
function(cutter_add_unit_test TEST_NAME)
    add_executable(${TEST_NAME} "${TEST_NAME}.cpp")

    target_include_directories(${TEST_NAME} PRIVATE ${TEST_INCLUDES} ${CMAKE_SOURCE_DIR}/test/common)

    set_target_properties(${TEST_NAME} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
    )

    target_link_libraries(${TEST_NAME} PRIVATE
        cutter_app
        Qt6::Test
        Qt6::Widgets
    )

    # test name will be shown in this format on output
    add_test(NAME unit.${TEST_NAME} COMMAND ${TEST_NAME})

    # all tests in this directory can be ran via: ctest -L unit
    set_tests_properties(unit.${TEST_NAME} PROPERTIES
        LABELS "unit"
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 120
    )
endfunction()

function(cutter_add_unit_tests)
    add_custom_target(tests_unit)
    add_dependencies(tests tests_unit)
    foreach(TEST_NAME IN LISTS ARGN)
        cutter_add_unit_test(${TEST_NAME})
        add_dependencies(tests_unit ${TEST_NAME})
    endforeach()
endfunction()

# Add unit tests here (preferably alphabetically sorted)
cutter_add_unit_tests(
    test_AbstractFilterView
    test_AddressableFilterProxy
    test_BasicInstructionHighlighter
    test_BinaryTrees
    test_DisassemblyHelper
    test_GraphLayouts
    test_Helpers
    test_Json
    test_LinkedListPool
    test_MdHighlighter
    test_RichTextPainter
    test_SelectionHighlight
    test_ShortcutManager
    test_SyntaxHighlighter
)
