file(GLOB TS_FILES
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    "translations/*/cutter_*.ts"
)

list(REMOVE_ITEM TS_FILES
    # problems with fonts
    translations/ko/cutter_ko.ts
    # #2321 handling multiple versions of a language
    translations/pt-BR/cutter_pt.ts
)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
if (CUTTER_QT6)
    find_package(Qt6LinguistTools REQUIRED)
    qt6_add_translation(qmFiles ${TS_FILES})
else()
    find_package(Qt5LinguistTools REQUIRED)
    qt5_add_translation(qmFiles ${TS_FILES})
endif()
add_custom_target(translations ALL DEPENDS ${qmFiles} SOURCES ${TS_FILES})

install(FILES
    ${qmFiles}
    # For Linux it might be more correct to use ${MAKE_INSTALL_LOCALEDIR}, but that
    # uses share/locale_name/software_name layout instead of share/software_name/locale_files.
    DESTINATION ${CUTTER_INSTALL_DATADIR}/translations
)

