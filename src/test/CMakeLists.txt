
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Test)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CutterTest.pro"
        "${CMAKE_CURRENT_BINARY_DIR}/CutterTest.pro"
        COPYONLY) # trigger reconfigure if CutterTest.pro changes
parse_qmake_pro("${CMAKE_CURRENT_BINARY_DIR}/CutterTest.pro" CUTTER_TEST_PRO)
set(TEST_SOURCE_FILES ${CUTTER_TEST_PRO_SOURCES})
set(TEST_HEADER_FILES ${CUTTER_TEST_PRO_HEADERS})

message(STATUS "sources from CutterTest.pro: ${TEST_SOURCE_FILES}")
message(STATUS "headers from CutterTest.pro: ${TEST_HEADER_FILES}")

add_executable(CutterTest ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES})
qt5_use_modules(CutterTest Core Widgets Gui Test)
