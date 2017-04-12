
cmake_minimum_required(VERSION 3.1)

project(Iaito VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui WebKit WebKitWidgets)


if(WIN32)
    # use radare2 libraries from submodule on windows
    set(IAITO_WIN32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iaito_win32")
    list(APPEND CMAKE_PREFIX_PATH "${IAITO_WIN32_DIR}")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        list(APPEND CMAKE_LIBRARY_PATH "${IAITO_WIN32_DIR}/radare2/lib64")
    else()
        list(APPEND CMAKE_LIBRARY_PATH "${IAITO_WIN32_DIR}/radare2/lib32")
    endif()

    set(RADARE2_INCLUDE_DIRS "${IAITO_WIN32_DIR}/radare2/include/libr" "${IAITO_WIN32_DIR}/include")
endif()


find_package(Radare2 REQUIRED)
include_directories(${RADARE2_INCLUDE_DIRS})


set(SOURCE_FILES
    main.cpp
    mainwindow.cpp
    newfiledialog.cpp
    optionsdialog.cpp
    highlighter.cpp
    qrcore.cpp
    createnewdialog.cpp
    hexascii_highlighter.cpp
    webserverthread.cpp
    widgets/pieview.cpp
    widgets/sectionswidget.cpp
    widgets/codegraphic.cpp
    widgets/notepad.cpp
    mdhighlighter.cpp
    widgets/functionswidget.cpp
    dialogs/renamedialog.cpp
    dialogs/aboutdialog.cpp
    widgets/importswidget.cpp
    widgets/symbolswidget.cpp
    widgets/relocswidget.cpp
    widgets/commentswidget.cpp
    widgets/stringswidget.cpp
    widgets/flagswidget.cpp
    widgets/memorywidget.cpp
    qrdisasm.cpp
    widgets/sdbdock.cpp
    analthread.cpp
    dialogs/commentsdialog.cpp
    widgets/sidebar.cpp
    helpers.cpp
    widgets/omnibar.cpp
    widgets/dashboard.cpp
    dialogs/xrefsdialog.cpp
    hexhighlighter.cpp)

set(HEADER_FILES
    mainwindow.h
    newfiledialog.h
    optionsdialog.h
    highlighter.h
    qrcore.h
    createnewdialog.h
    hexascii_highlighter.h
    webserverthread.h
    widgets/pieview.h
    widgets/sectionswidget.h
    widgets/codegraphic.h
    widgets/notepad.h
    mdhighlighter.h
    widgets/functionswidget.h
    dialogs/renamedialog.h
    dialogs/aboutdialog.h
    widgets/importswidget.h
    widgets/symbolswidget.h
    widgets/relocswidget.h
    widgets/commentswidget.h
    widgets/stringswidget.h
    widgets/flagswidget.h
    widgets/memorywidget.h
    qrdisasm.h
    widgets/sdbdock.h
    analthread.h
    dialogs/commentsdialog.h
    widgets/sidebar.h
    helpers.h
    widgets/omnibar.h
    widgets/dashboard.h
    dialogs/xrefsdialog.h
    widgets/banned.h
    hexhighlighter.h)

set(UI_FILES
    mainwindow.ui
    newfiledialog.ui
    optionsdialog.ui
    createnewdialog.ui
    widgets/notepad.ui
    widgets/functionswidget.ui
    dialogs/aboutdialog.ui
    dialogs/renamedialog.ui
    widgets/importswidget.ui
    widgets/symbolswidget.ui
    widgets/relocswidget.ui
    widgets/commentswidget.ui
    widgets/stringswidget.ui
    widgets/flagswidget.ui
    widgets/memorywidget.ui
    widgets/sdbdock.ui
    dialogs/commentsdialog.ui
    widgets/sidebar.ui
    widgets/dashboard.ui
    dialogs/xrefsdialog.ui)

set(QRC_FILES resources.qrc)


set(IAITO_VERSION_SUFFIX "-dev")
set(IAITO_VERSION_FULL "${PROJECT_VERSION}${IAITO_VERSION_SUFFIX}")
message("Building Iaito version ${IAITO_VERSION_FULL}")
add_definitions("-DAPP_VERSION=\"${IAITO_VERSION_FULL}\"")


add_executable(iaito ${UI_FILES} ${QRC_FILES} ${SOURCE_FILES} ${HEADER_FILES})
qt5_use_modules(iaito Core Widgets Gui WebKit WebKitWidgets)
target_link_libraries(iaito ${RADARE2_LIBRARIES})
