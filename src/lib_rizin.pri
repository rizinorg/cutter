win32 {
    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    LIBS += -L"$$PWD/../rz_dist/lib"
    RZ_INCLUDEPATH += "$$PWD/../rz_dist/include/librz"
    RZ_INCLUDEPATH += "$$PWD/../rz_dist/include/librz/sdb"
    INCLUDEPATH += $$RZ_INCLUDEPATH

    LIBS += \
        -lrz_core \
        -lrz_config \
        -lrz_cons \
        -lrz_io \
        -lrz_util \
        -lrz_flag \
        -lrz_asm \
        -lrz_debug \
        -lrz_hash \
        -lrz_bin \
        -lrz_lang \
        -lrz_analysis \
        -lrz_parse \
        -lrz_bp \
        -lrz_egg \
        -lrz_reg \
        -lrz_search \
        -lrz_syscall \
        -lrz_socket \
        -lrz_fs \
        -lrz_magic \
        -lrz_crypto
} else {
    macx|bsd {
        RZPREFIX=/usr/local
    } else {
        RZPREFIX=/usr
    }
    USE_PKGCONFIG = 1
    RZ_USER_PKGCONFIG = $$(HOME)/bin/prefix/rizin/lib/pkgconfig
    exists($$RZ_USER_PKGCONFIG) {
        # caution: may not work for cross compilations
        PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:$$RZ_USER_PKGCONFIG
    } else {
        unix {
            exists($$RZPREFIX/lib/pkgconfig/rz_core.pc) {
                PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:$$RZPREFIX/lib/pkgconfig
            } else {
                LIBS += -L$$RZPREFIX/lib
                RZ_INCLUDEPATH += $$RZPREFIX/include/librz
                RZ_INCLUDEPATH += $$RZPREFIX/include/librz/sdb
                USE_PKGCONFIG = 0
            }
        }
        macx {
            LIBS += -L$$RZPREFIX/lib
            RZ_INCLUDEPATH += $$RZPREFIX/include/librz
            RZ_INCLUDEPATH += $$RZPREFIX/include/librz/sdb
            USE_PKGCONFIG = 0
        }
        bsd {
            !exists($$PKG_CONFIG_PATH/rz_core.pc) {
                LIBS += -L$$RZPREFIX/lib
                RZ_INCLUDEPATH += $$RZPREFIX/include/librz
                RZ_INCLUDEPATH += $$RZPREFIX/include/librz/sdb
                USE_PKGCONFIG = 0
            }
        }
    }
    INCLUDEPATH += $$RZ_INCLUDEPATH

    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    equals(USE_PKGCONFIG, 1) {
        CONFIG += link_pkgconfig
        PKGCONFIG += rz_core
        RZ_INCLUDEPATH = "$$system("pkg-config --variable=includedir rz_core")/librz"
        RZ_INCLUDEPATH += "$$system("pkg-config --variable=includedir rz_core")/librz/sdb"
    } else {
        LIBS += \
        -lrz_core \
        -lrz_config \
        -lrz_cons \
        -lrz_io \
        -lrz_flag \
        -lrz_asm \
        -lrz_debug \
        -lrz_hash \
        -lrz_bin \
        -lrz_lang \
        -lrz_parse \
        -lrz_bp \
        -lrz_egg \
        -lrz_reg \
        -lrz_search \
        -lrz_syscall \
        -lrz_socket \
        -lrz_fs \
        -lrz_analysis \
        -lrz_magic \
        -lrz_util \
        -lrz_crypto
    }
}
