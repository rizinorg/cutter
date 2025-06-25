configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CutterConfig.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/CutterConfig.h")

set(SOURCES
    Main.cpp
    core/Cutter.cpp
    core/CutterJson.cpp
    core/RizinCpp.cpp
    core/Basefind.cpp
    dialogs/EditStringDialog.cpp
    dialogs/WriteCommandsDialogs.cpp
    widgets/DisassemblerGraphView.cpp
    widgets/OverviewView.cpp
    common/DisassemblyPreview.cpp
    common/RichTextPainter.cpp
    dialogs/InitialOptionsDialog.cpp
    dialogs/AboutDialog.cpp
    dialogs/CommentsDialog.cpp
    dialogs/EditInstructionDialog.cpp
    dialogs/FlagDialog.cpp
    dialogs/GlobalVariableDialog.cpp
    dialogs/RemoteDebugDialog.cpp
    dialogs/NativeDebugDialog.cpp
    dialogs/XrefsDialog.cpp
    core/MainWindow.cpp
    common/Helpers.cpp
    common/Highlighter.cpp
    common/MdHighlighter.cpp
    common/DirectionalComboBox.cpp
    dialogs/preferences/AsmOptionsWidget.cpp
    dialogs/NewFileDialog.cpp
    common/AnalysisTask.cpp
    widgets/CommentsWidget.cpp
    widgets/ConsoleWidget.cpp
    widgets/Dashboard.cpp
    widgets/EntrypointWidget.cpp
    widgets/ExportsWidget.cpp
    widgets/FlagsWidget.cpp
    widgets/FunctionsWidget.cpp
    widgets/GlobalsWidget.cpp
    widgets/ImportsWidget.cpp
    widgets/Omnibar.cpp
    widgets/RelocsWidget.cpp
    widgets/SectionsWidget.cpp
    widgets/SegmentsWidget.cpp
    widgets/StringsWidget.cpp
    widgets/SymbolsWidget.cpp
    menus/DisassemblyContextMenu.cpp
    menus/DecompilerContextMenu.cpp
    menus/FlirtContextMenu.cpp
    widgets/DisassemblyWidget.cpp
    widgets/HexdumpWidget.cpp
    common/Configuration.cpp
    common/Colors.cpp
    common/TempConfig.cpp
    common/SvgIconEngine.cpp
    common/SyntaxHighlighter.cpp
    widgets/DecompilerWidget.cpp
    widgets/VisualNavbar.cpp
    widgets/GraphView.cpp
    dialogs/preferences/PreferencesDialog.cpp
    dialogs/preferences/AppearanceOptionsWidget.cpp
    dialogs/preferences/GraphOptionsWidget.cpp
    dialogs/preferences/PreferenceCategory.cpp
    dialogs/preferences/InitializationFileEditor.cpp
    widgets/QuickFilterView.cpp
    widgets/ClassesWidget.cpp
    widgets/ResourcesWidget.cpp
    widgets/VTablesWidget.cpp
    widgets/TypesWidget.cpp
    widgets/HeadersWidget.cpp
    widgets/SearchWidget.cpp
    CutterApplication.cpp
    dialogs/RizinPluginsDialog.cpp
    widgets/CutterDockWidget.cpp
    widgets/CutterTreeWidget.cpp
    widgets/GraphWidget.cpp
    widgets/OverviewWidget.cpp
    common/JsonModel.cpp
    dialogs/VersionInfoDialog.cpp
    widgets/FlirtWidget.cpp
    common/AsyncTask.cpp
    dialogs/AsyncTaskDialog.cpp
    widgets/StackWidget.cpp
    widgets/RegistersWidget.cpp
    widgets/ThreadsWidget.cpp
    widgets/ProcessesWidget.cpp
    widgets/BacktraceWidget.cpp
    dialogs/MapFileDialog.cpp
    common/CommandTask.cpp
    common/ProgressIndicator.cpp
    common/RizinTask.cpp
    dialogs/RizinTaskDialog.cpp
    widgets/DebugActions.cpp
    widgets/MemoryMapWidget.cpp
    dialogs/preferences/DebugOptionsWidget.cpp
    dialogs/preferences/PluginsOptionsWidget.cpp
    widgets/BreakpointWidget.cpp
    dialogs/BreakpointsDialog.cpp
    dialogs/AttachProcDialog.cpp
    widgets/RegisterRefsWidget.cpp
    dialogs/SetToDataDialog.cpp
    dialogs/EditVariablesDialog.cpp
    dialogs/EditFunctionDialog.cpp
    widgets/CutterTreeView.cpp
    widgets/ComboQuickFilterView.cpp
    dialogs/HexdumpRangeDialog.cpp
    common/CutterSeekable.cpp
    common/RefreshDeferrer.cpp
    dialogs/WelcomeDialog.cpp
    common/RunScriptTask.cpp
    dialogs/EditMethodDialog.cpp
    dialogs/TypesInteractionDialog.cpp
    widgets/SdbWidget.cpp
    plugins/PluginManager.cpp
    common/BasicBlockHighlighter.cpp
    common/BasicInstructionHighlighter.cpp
    widgets/ColorPicker.cpp
    common/ColorThemeWorker.cpp
    widgets/ColorThemeComboBox.cpp
    widgets/ColorThemeListView.cpp
    dialogs/preferences/ColorThemeEditDialog.cpp
    common/UpdateWorker.cpp
    widgets/MemoryDockWidget.cpp
    common/BugReporting.cpp
    common/HighDpiPixmap.cpp
    widgets/GraphGridLayout.cpp
    widgets/HexWidget.cpp
    common/SelectionHighlight.cpp
    common/Decompiler.cpp
    menus/AddressableItemContextMenu.cpp
    common/AddressableItemModel.cpp
    widgets/ListDockWidget.cpp
    dialogs/MultitypeFileSaveDialog.cpp
    widgets/BoolToggleDelegate.cpp
    common/IOModesController.cpp
    common/SettingsUpgrade.cpp
    dialogs/LayoutManager.cpp
    common/CutterLayout.cpp
    widgets/GraphHorizontalAdapter.cpp
    common/ResourcePaths.cpp
    widgets/CutterGraphView.cpp
    widgets/SimpleTextGraphView.cpp
    widgets/RizinGraphWidget.cpp
    widgets/CallGraph.cpp
    widgets/AddressableDockWidget.cpp
    dialogs/preferences/AnalysisOptionsWidget.cpp
    common/DecompilerHighlighter.cpp
    dialogs/GlibcHeapInfoDialog.cpp
    widgets/HeapDockWidget.cpp
    widgets/GlibcHeapWidget.cpp
    dialogs/GlibcHeapBinsDialog.cpp
    widgets/HeapBinsGraphView.cpp
    dialogs/ArenaInfoDialog.cpp
    tools/basefind/BaseFindDialog.cpp
    tools/basefind/BaseFindSearchDialog.cpp
    tools/basefind/BaseFindResultsDialog.cpp
    dialogs/preferences/ShortcutOptionsWidget.cpp
    shortcuts/ShortcutManager.cpp
    shortcuts/DefaultShortcuts.cpp
)
set(HEADER_FILES
    core/Cutter.h
    core/CutterCommon.h
    core/CutterDescriptions.h
    core/CutterJson.h
    core/RizinCpp.h
    core/Basefind.h
    dialogs/EditStringDialog.h
    dialogs/WriteCommandsDialogs.h
    widgets/DisassemblerGraphView.h
    widgets/OverviewView.h
    common/RichTextPainter.h
    common/CachedFontMetrics.h
    dialogs/AboutDialog.h
    dialogs/preferences/AsmOptionsWidget.h
    dialogs/CommentsDialog.h
    dialogs/EditInstructionDialog.h
    dialogs/FlagDialog.h
    dialogs/GlobalVariableDialog.h
    dialogs/RemoteDebugDialog.h
    dialogs/NativeDebugDialog.h
    dialogs/XrefsDialog.h
    common/Helpers.h
    core/MainWindow.h
    common/Highlighter.h
    common/MdHighlighter.h
    common/DirectionalComboBox.h
    dialogs/InitialOptionsDialog.h
    dialogs/NewFileDialog.h
    common/AnalysisTask.h
    widgets/CommentsWidget.h
    widgets/ConsoleWidget.h
    widgets/Dashboard.h
    widgets/EntrypointWidget.h
    widgets/ExportsWidget.h
    widgets/FlagsWidget.h
    widgets/FunctionsWidget.h
    widgets/ImportsWidget.h
    widgets/Omnibar.h
    widgets/RelocsWidget.h
    widgets/SectionsWidget.h
    widgets/SegmentsWidget.h
    widgets/StringsWidget.h
    widgets/SymbolsWidget.h
    menus/DisassemblyContextMenu.h
    menus/DecompilerContextMenu.h
    menus/FlirtContextMenu.h
    widgets/DisassemblyWidget.h
    widgets/HexdumpWidget.h
    common/Configuration.h
    common/Colors.h
    common/TempConfig.h
    common/SvgIconEngine.h
    common/SyntaxHighlighter.h
    widgets/DecompilerWidget.h
    widgets/VisualNavbar.h
    widgets/GraphView.h
    dialogs/preferences/PreferencesDialog.h
    dialogs/preferences/AppearanceOptionsWidget.h
    dialogs/preferences/PreferenceCategory.h
    dialogs/preferences/GraphOptionsWidget.h
    dialogs/preferences/InitializationFileEditor.h
    widgets/QuickFilterView.h
    widgets/ClassesWidget.h
    widgets/ResourcesWidget.h
    CutterApplication.h
    widgets/VTablesWidget.h
    widgets/TypesWidget.h
    widgets/HeadersWidget.h
    widgets/SearchWidget.h
    dialogs/RizinPluginsDialog.h
    widgets/CutterDockWidget.h
    widgets/CutterTreeWidget.h
    widgets/GraphWidget.h
    widgets/OverviewWidget.h
    common/JsonModel.h
    dialogs/VersionInfoDialog.h
    widgets/FlirtWidget.h
    common/AsyncTask.h
    dialogs/AsyncTaskDialog.h
    widgets/StackWidget.h
    widgets/RegistersWidget.h
    widgets/ThreadsWidget.h
    widgets/ProcessesWidget.h
    widgets/BacktraceWidget.h
    dialogs/MapFileDialog.h
    common/StringsTask.h
    common/FunctionsTask.h
    common/CommandTask.h
    common/ProgressIndicator.h
    plugins/CutterPlugin.h
    common/RizinTask.h
    dialogs/RizinTaskDialog.h
    widgets/DebugActions.h
    widgets/MemoryMapWidget.h
    dialogs/preferences/DebugOptionsWidget.h
    dialogs/preferences/PluginsOptionsWidget.h
    widgets/BreakpointWidget.h
    dialogs/BreakpointsDialog.h
    dialogs/AttachProcDialog.h
    widgets/RegisterRefsWidget.h
    dialogs/SetToDataDialog.h
    common/InitialOptions.h
    dialogs/EditVariablesDialog.h
    dialogs/EditFunctionDialog.h
    widgets/CutterTreeView.h
    widgets/ComboQuickFilterView.h
    dialogs/HexdumpRangeDialog.h
    common/CutterSeekable.h
    common/RefreshDeferrer.h
    dialogs/WelcomeDialog.h
    common/RunScriptTask.h
    common/Json.h
    dialogs/EditMethodDialog.h
    dialogs/TypesInteractionDialog.h
    widgets/SdbWidget.h
    plugins/PluginManager.h
    common/BasicBlockHighlighter.h
    common/BasicInstructionHighlighter.h
    common/UpdateWorker.h
    widgets/ColorPicker.h
    common/ColorThemeWorker.h
    widgets/ColorThemeComboBox.h
    widgets/MemoryDockWidget.h
    widgets/ColorThemeListView.h
    dialogs/preferences/ColorThemeEditDialog.h
    common/BugReporting.h
    common/HighDpiPixmap.h
    widgets/GraphLayout.h
    widgets/GraphGridLayout.h
    widgets/HexWidget.h
    common/SelectionHighlight.h
    common/Decompiler.h
    menus/AddressableItemContextMenu.h
    common/AddressableItemModel.h
    widgets/ListDockWidget.h
    widgets/AddressableItemList.h
    dialogs/MultitypeFileSaveDialog.h
    widgets/BoolToggleDelegate.h
    common/IOModesController.h
    common/SettingsUpgrade.h
    dialogs/LayoutManager.h
    common/CutterLayout.h
    common/BinaryTrees.h
    common/LinkedListPool.h
    widgets/GraphHorizontalAdapter.h
    common/ResourcePaths.h
    widgets/CutterGraphView.h
    widgets/SimpleTextGraphView.h
    widgets/RizinGraphWidget.h
    widgets/CallGraph.h
    widgets/AddressableDockWidget.h
    dialogs/preferences/AnalysisOptionsWidget.h
    common/DecompilerHighlighter.h
    dialogs/GlibcHeapInfoDialog.h
    widgets/HeapDockWidget.h
    widgets/GlibcHeapWidget.h
    dialogs/GlibcHeapBinsDialog.h
    widgets/HeapBinsGraphView.h
    dialogs/ArenaInfoDialog.h
    tools/basefind/BaseFindDialog.h
    tools/basefind/BaseFindSearchDialog.h
    tools/basefind/BaseFindResultsDialog.h
    dialogs/preferences/ShortcutOptionsWidget.h
    shortcuts/ShortcutManager.h
    shortcuts/DefaultShortcuts.h
)
set(UI_FILES
    dialogs/AboutDialog.ui
    dialogs/EditStringDialog.ui
    dialogs/Base64EnDecodedWriteDialog.ui
    dialogs/DuplicateFromOffsetDialog.ui
    dialogs/IncrementDecrementDialog.ui
    dialogs/preferences/AsmOptionsWidget.ui
    dialogs/CommentsDialog.ui
    dialogs/EditInstructionDialog.ui
    dialogs/FlagDialog.ui
    dialogs/GlobalVariableDialog.ui
    dialogs/RemoteDebugDialog.ui
    dialogs/NativeDebugDialog.ui
    dialogs/XrefsDialog.ui
    dialogs/NewFileDialog.ui
    dialogs/InitialOptionsDialog.ui
    dialogs/EditFunctionDialog.ui
    core/MainWindow.ui
    widgets/ConsoleWidget.ui
    widgets/Dashboard.ui
    widgets/EntrypointWidget.ui
    widgets/FlagsWidget.ui
    widgets/GlobalsWidget.ui
    widgets/StringsWidget.ui
    widgets/HexdumpWidget.ui
    dialogs/preferences/PreferencesDialog.ui
    dialogs/preferences/AppearanceOptionsWidget.ui
    dialogs/preferences/GraphOptionsWidget.ui
    dialogs/preferences/InitializationFileEditor.ui
    widgets/QuickFilterView.ui
    widgets/DecompilerWidget.ui
    widgets/VTablesWidget.ui
    widgets/TypesWidget.ui
    widgets/SearchWidget.ui
    dialogs/RizinPluginsDialog.ui
    dialogs/VersionInfoDialog.ui
    widgets/FlirtWidget.ui
    dialogs/AsyncTaskDialog.ui
    dialogs/RizinTaskDialog.ui
    widgets/StackWidget.ui
    widgets/RegistersWidget.ui
    widgets/ThreadsWidget.ui
    widgets/ProcessesWidget.ui
    widgets/BacktraceWidget.ui
    dialogs/MapFileDialog.ui
    dialogs/preferences/DebugOptionsWidget.ui
    widgets/BreakpointWidget.ui
    dialogs/BreakpointsDialog.ui
    dialogs/AttachProcDialog.ui
    widgets/RegisterRefsWidget.ui
    dialogs/SetToDataDialog.ui
    dialogs/EditVariablesDialog.ui
    widgets/CutterTreeView.ui
    widgets/ComboQuickFilterView.ui
    dialogs/HexdumpRangeDialog.ui
    dialogs/WelcomeDialog.ui
    dialogs/EditMethodDialog.ui
    dialogs/TypesInteractionDialog.ui
    widgets/SdbWidget.ui
    widgets/ColorPicker.ui
    dialogs/preferences/ColorThemeEditDialog.ui
    widgets/ListDockWidget.ui
    dialogs/LayoutManager.ui
    widgets/RizinGraphWidget.ui
    dialogs/preferences/AnalysisOptionsWidget.ui
    dialogs/GlibcHeapInfoDialog.ui
    widgets/HeapDockWidget.ui
    widgets/GlibcHeapWidget.ui
    dialogs/GlibcHeapBinsDialog.ui
    dialogs/ArenaInfoDialog.ui
    tools/basefind/BaseFindDialog.ui
    tools/basefind/BaseFindSearchDialog.ui
    tools/basefind/BaseFindResultsDialog.ui
    dialogs/preferences/ShortcutOptionsWidget.ui
)
set(QRC_FILES
    resources.qrc
    themes/native/native.qrc
    themes/qdarkstyle/dark.qrc
    themes/midnight/midnight.qrc
    themes/lightstyle/light.qrc
)

set(CUTTER_INCLUDE_DIRECTORIES core widgets common plugins menus .)

if (CUTTER_ENABLE_PYTHON)
    list(APPEND SOURCES common/QtResImporter.cpp common/PythonManager.cpp common/PythonAPI.cpp)
    list(APPEND HEADER_FILES common/QtResImporter.h common/PythonManager.h common/PythonAPI.h)
endif()

if(CUTTER_ENABLE_PYTHON_BINDINGS)
    if (CUTTER_QT EQUAL 6)
        set(PYSIDE_NAME PySide6)
        set(SHIBOKEN_COMMAND Shiboken6::shiboken6)
        set (CUTTER_PYSIDE_LIBS Shiboken6::libshiboken PySide6::pyside6)
    elseif(CUTTER_QT EQUAL 5)
        set(PYSIDE_NAME PySide2)
        set(SHIBOKEN_COMMAND Shiboken2::shiboken2)
        set (CUTTER_PYSIDE_LIBS Shiboken2::libshiboken PySide2::pyside2)
    endif()

    set(BINDINGS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bindings")
    set(BINDINGS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/bindings")

    configure_file("${BINDINGS_SRC_DIR}/bindings.xml.in" "${BINDINGS_BUILD_DIR}/bindings.xml")

    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${BINDINGS_SRC_DIR}/src_list.py" cmake "${BINDINGS_BUILD_DIR}" OUTPUT_VARIABLE BINDINGS_SOURCE)

    set_property(SOURCE ${BINDINGS_SOURCE} PROPERTY SKIP_AUTOGEN ON)

    include_directories("${BINDINGS_BUILD_DIR}/CutterBindings")

    set(SHIBOKEN_OPTIONS)
    if (WIN32)
        list(APPEND SHIBOKEN_OPTIONS --avoid-protected-hack)
    endif()

    add_custom_command(OUTPUT ${BINDINGS_SOURCE}
            COMMAND "${SHIBOKEN_COMMAND}" --project-file="${BINDINGS_BUILD_DIR}/bindings.txt" ${SHIBOKEN_OPTIONS} ${SHIBOKEN_EXTRA_OPTIONS}
            DEPENDS "${BINDINGS_BUILD_DIR}/bindings.xml" "${BINDINGS_BUILD_DIR}/bindings.txt"
            IMPLICIT_DEPENDS CXX "${CMAKE_CURRENT_SOURCE_DIR}/bindings/bindings.h"
            COMMENT "Generating Python bindings with ${SHIBOKEN_COMMAND}")
else()
    set(BINDINGS_SOURCE "")
endif()


if (TARGET Graphviz::GVC)
    list(APPEND SOURCES widgets/GraphvizLayout.cpp)
    list(APPEND HEADER_FILES widgets/GraphvizLayout.h)
endif()

if (WIN32)
    set(PLATFORM_RESOURCES "img/cutter.rc")
    set(OPTIONS WIN32)
else()
    set(PLATFORM_RESOURCES "")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(-Wall -Wextra)
    set_source_files_properties(${BINDINGS_SOURCE} PROPERTIES COMPILE_FLAGS -w)
endif()

# Make a source group for Visual Studio
set(CUTTER_SOURCES ${OPTIONS} ${UI_FILES} ${QRC_FILES} ${PLATFORM_RESOURCES} ${SOURCES} ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${CUTTER_SOURCES})

add_executable(Cutter ${CUTTER_SOURCES} ${BINDINGS_SOURCE})
set_target_properties(Cutter PROPERTIES
        OUTPUT_NAME cutter
        RUNTIME_OUTPUT_DIRECTORY ..
        PDB_OUTPUT_DIRECTORY ..
        ENABLE_EXPORTS ON
        CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(Cutter PRIVATE CUTTER_SOURCE_BUILD)

# Set Cutter as the startup project in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Cutter)

foreach(_dir ${CUTTER_INCLUDE_DIRECTORIES})
    target_include_directories(Cutter PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${_dir}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cutter/${_dir}>
    )
endforeach()

if (TARGET Graphviz::GVC)
    target_link_libraries(Cutter PRIVATE Graphviz::GVC)
    target_compile_definitions(Cutter PRIVATE CUTTER_ENABLE_GRAPHVIZ)
endif()

target_link_libraries(Cutter PUBLIC ${QT_PREFIX}::Core ${QT_PREFIX}::Widgets ${QT_PREFIX}::Gui PRIVATE  ${QT_PREFIX}::Svg ${QT_PREFIX}::Network)
if (CUTTER_QT EQUAL 6)
    target_link_libraries(Cutter PUBLIC Qt6::Core5Compat Qt6::SvgWidgets)
    target_link_libraries(Cutter PRIVATE Qt6::OpenGLWidgets)
endif()

target_link_libraries(Cutter PUBLIC ${RIZIN_TARGET})
if(CUTTER_ENABLE_PYTHON)
    if(CMAKE_VERSION VERSION_LESS "3.12.0") 
        # On windows some of the Python STABLE API functions are in seperate library
        # which isn't added by CMake.
        if (WIN32)
            foreach(_PYTHON_LIBRARY ${PYTHON_LIBRARIES})
                # Skip extra "optimized" and "debug" values that are only meant for passing to target_link_libraries()
                if((NOT _PYTHON_LIBRARY STREQUAL "optimized") AND (NOT _PYTHON_LIBRARY STREQUAL "debug"))
                    get_filename_component(_PYTHON_LIB_DIR ${_PYTHON_LIBRARY} DIRECTORY)
                    message(STATUS "Add extra library dir for Python: ${_PYTHON_LIB_DIR}")
                    target_link_directories(Cutter PRIVATE ${_PYTHON_LIB_DIR})
                endif()
            endforeach()
        endif()
        target_link_libraries(Cutter PRIVATE ${PYTHON_LIBRARIES})
    else()
        target_link_libraries(Cutter PRIVATE Python3::Python)
    endif()

    if(CUTTER_ENABLE_PYTHON_BINDINGS)
        target_link_libraries(Cutter PRIVATE ${CUTTER_PYSIDE_LIBS})

        get_target_property(RAW_BINDINGS_INCLUDE_DIRS Cutter INCLUDE_DIRECTORIES)
        if(NOT CUTTER_USE_BUNDLED_RIZIN)
            get_target_property(RAW_RIZIN_INCLUDE_DIRS ${RIZIN_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
            list(APPEND RAW_BINDINGS_INCLUDE_DIRS "${RAW_RIZIN_INCLUDE_DIRS}")
        endif()
        set(BINDINGS_INCLUDE_DIRS "")
        foreach(_dir ${RAW_BINDINGS_INCLUDE_DIRS})
            string(REGEX REPLACE "\\$<BUILD_INTERFACE:(.*)>" "\\1" _dir ${_dir})
            string(REGEX REPLACE "\\$<INSTALL_INTERFACE:(.*)>" "" _dir ${_dir})
            if (NOT "${_dir}" STREQUAL "")
                list(APPEND BINDINGS_INCLUDE_DIRS "${_dir}")
            endif()
        endforeach()

        set(BINDINGS_INCLUDE_DIR_LINES "")
        if(APPLE)
            if (_qt5Core_install_prefix)
                list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include")
                list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtCore")
                list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtGui")
                list(APPEND BINDINGS_INCLUDE_DIRS "${_qt5Core_install_prefix}/include/QtWidgets")
            elseif(CUTTER_QT EQUAL 6)
                string(APPEND BINDINGS_INCLUDE_DIR_LINES "framework-include-path=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}\n")
            endif()
        endif()
        list(APPEND BINDINGS_INCLUDE_DIRS ${${QT_PREFIX}Core_INCLUDE_DIRS} ${${QT_PREFIX}Widgets_INCLUDE_DIRS} ${${QT_PREFIX}Gui_INCLUDE_DIRS})
        list(APPEND BINDINGS_INCLUDE_DIRS ${Rizin_INCLUDE_DIRS})
        list(APPEND BINDINGS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")

        foreach(_dir ${BINDINGS_INCLUDE_DIRS})
            if (NOT "${_dir}" STREQUAL "")
                string(APPEND BINDINGS_INCLUDE_DIR_LINES "include-path = ${_dir}\n")
            endif()
        endforeach()

        configure_file("${BINDINGS_SRC_DIR}/bindings.txt.in" "${BINDINGS_BUILD_DIR}/bindings.txt")
        add_definitions(-DWIN32_LEAN_AND_MEAN)
    endif()
endif()

if(TARGET KF${CUTTER_QT}::SyntaxHighlighting)
    target_link_libraries(Cutter PRIVATE KF${CUTTER_QT}::SyntaxHighlighting)
    target_compile_definitions(Cutter PRIVATE CUTTER_ENABLE_KSYNTAXHIGHLIGHTING)
endif()

if (CUTTER_APPIMAGE_BUILD)
    target_compile_definitions(Cutter PRIVATE APPIMAGE)
endif()

if (CUTTER_PACKAGE_JSDEC)
	target_compile_definitions(Cutter PRIVATE CUTTER_BUNDLE_JSDEC)
endif()

if(APPLE AND CUTTER_ENABLE_PACKAGING AND CUTTER_USE_BUNDLED_RIZIN)
	target_compile_definitions(Cutter PRIVATE MACOS_RZ_BUNDLED)
endif()

if(CUTTER_ENABLE_PACKAGING)
    target_compile_definitions(Cutter PRIVATE CUTTER_ENABLE_PACKAGING)
endif()

include(Translations)

# Install files
install(TARGETS Cutter
        EXPORT CutterTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        BUNDLE DESTINATION "." # needs to be tested
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Devel)
install(EXPORT CutterTargets
    NAMESPACE Cutter::
    DESTINATION "${CUTTER_INSTALL_CONFIGDIR}"
    COMPONENT Devel)
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CutterConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/CutterConfig.cmake"
    INSTALL_DESTINATION "${CUTTER_INSTALL_CONFIGDIR}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CutterConfigVersion.cmake"
    VERSION "${CUTTER_VERSION_FULL}"
    COMPATIBILITY AnyNewerVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CutterConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CutterConfigVersion.cmake"
    DESTINATION ${CUTTER_INSTALL_CONFIGDIR}
    COMPONENT Devel)
foreach(_file ${HEADER_FILES})
    # Can't use target PUBLIC_HEADER option for installing due to multiple directories
    get_filename_component(_header_dir "${_file}" DIRECTORY)
    install (FILES "${_file}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cutter/${_header_dir}" COMPONENT Devel)
endforeach()

if(UNIX AND NOT APPLE)
    install(FILES "img/cutter.svg"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps")
    install(FILES "re.rizin.cutter.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    install(FILES "re.rizin.cutter.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")
endif()
