set(TS_FILES
    translations/cutter_ar.ts
    translations/cutter_ca.ts
    translations/cutter_cn.ts
    translations/cutter_de.ts
    translations/cutter_es.ts
    translations/cutter_fa.ts
    translations/cutter_fr.ts
    translations/cutter_he.ts
    translations/cutter_hi.ts
    translations/cutter_it.ts
    translations/cutter_ja.ts
    translations/cutter_nl.ts
    translations/cutter_pt.ts
    translations/cutter_ro.ts
    translations/cutter_ru.ts
    translations/cutter_tr.ts
)
# translations/cutter_ko.ts problems with fonts
# translations/cutter_pt-BR #2321 handling multiple versions of a language

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
find_package(Qt5LinguistTools REQUIRED)
qt5_add_translation(qmFiles ${TS_FILES})
add_custom_target(translations ALL DEPENDS ${qmFiles} SOURCES ${TS_FILES})

install(FILES
    ${qmFiles}
    # For Linux it might be more correct to use ${MAKE_INSTALL_LOCALEDIR}, but that
    # uses share/locale_name/software_name layout instead of share/software_name/locale_files.
    DESTINATION ${CUTTER_INSTALL_DATADIR}/translations
)

