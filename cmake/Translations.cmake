# Languages with 0% progress or low progress and alternatives are disabled
set(TS_FILES
#   translations/am/cutter_am_ET.ts
    translations/ar/cutter_ar_SA.ts
    translations/bn/cutter_bn_BD.ts
    translations/ca/cutter_ca_ES.ts
    translations/de/cutter_de_DE.ts
    translations/es-ES/cutter_es_ES.ts
    translations/fa/cutter_fa_IR.ts
    translations/fi/cutter_fi_FI.ts
#   translations/fil/cutter_fil_PH.ts
    translations/fr/cutter_fr_FR.ts
    translations/he/cutter_he_IL.ts
    translations/hi/cutter_hi_IN.ts
    translations/it/cutter_it_IT.ts
    translations/ja/cutter_ja_JP.ts
#   translations/km/cutter_km_KH.ts
    translations/ko/cutter_ko_KR.ts
    translations/nl/cutter_nl_NL.ts
#   translations/pa-IN/cutter_pa_IN.ts
    translations/pl/cutter_pl_PL.ts
#   translations/pt-BR/cutter_pt_BR.ts
    translations/pt-PT/cutter_pt_PT.ts
    translations/ro/cutter_ro_RO.ts
    translations/ru/cutter_ru_RU.ts
#   translations/sv-SE/cutter_sv_SE.ts
#   translations/sw/cutter_sw_KE.ts
#   translations/th/cutter_th_TH.ts
    translations/tr/cutter_tr_TR.ts
    translations/uk/cutter_uk_UA.ts
#   translations/ur-IN/cutter_ur_IN.ts
    translations/ur-PK/cutter_ur_PK.ts
    translations/vi/cutter_vi_VN.ts
    translations/zh-CN/cutter_zh_CN.ts
    translations/zh-TW/cutter_zh_TW.ts
)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
if (CUTTER_QT EQUAL 6)
    find_package(Qt6LinguistTools REQUIRED)
    qt6_add_translation(qmFiles ${TS_FILES})
elseif(CUTTER_QT EQUAL 5)
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

