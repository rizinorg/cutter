win32 {
    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    !contains(QT_ARCH, x86_64) {
        LIBS += -L"$$PWD/../r2_dist_x86/radare2/lib"
        R2_INCLUDEPATH += "$$PWD/../r2_dist_x86/include"
    } else {
        LIBS += -L"$$PWD/../r2_dist_x64/radare2/lib"
        R2_INCLUDEPATH += "$$PWD/../r2_dist_x64/include"
    }
    INCLUDEPATH += $$R2_INCLUDEPATH

    LIBS += \
        -lr_core \
        -lr_config \
        -lr_cons \
        -lr_io \
        -lr_util \
        -lr_flag \
        -lr_asm \
        -lr_debug \
        -lr_hash \
        -lr_bin \
        -lr_lang \
        -lr_anal \
        -lr_parse \
        -lr_bp \
        -lr_egg \
        -lr_reg \
        -lr_search \
        -lr_syscall \
        -lr_socket \
        -lr_fs \
        -lr_magic \
        -lr_crypto
} else {
    macx|bsd {
        R2PREFIX=/usr/local
    } else {
        R2PREFIX=/usr
    }
    USE_PKGCONFIG = 1
    R2_USER_PKGCONFIG = $$(HOME)/bin/prefix/radare2/lib/pkgconfig
    exists($$R2_USER_PKGCONFIG) {
        # caution: may not work for cross compilations
        PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:$$R2_USER_PKGCONFIG
    } else {
        unix {
            exists($$R2PREFIX/lib/pkgconfig/r_core.pc) {
                PKG_CONFIG_PATH=$$PKG_CONFIG_PATH:$$R2PREFIX/lib/pkgconfig
            } else {
                LIBS += -L$$R2PREFIX/lib
                R2_INCLUDEPATH += $$R2PREFIX/include/libr
                USE_PKGCONFIG = 0
            }
        }
        macx {
            LIBS += -L$$R2PREFIX/lib
            R2_INCLUDEPATH += $$R2PREFIX/include/libr
            USE_PKGCONFIG = 0
        }
        bsd {
            !exists($$PKG_CONFIG_PATH/r_core.pc) {
                LIBS += -L$$R2PREFIX/lib
                R2_INCLUDEPATH += $$R2PREFIX/include/libr
                USE_PKGCONFIG = 0
            }
        }
    }
    INCLUDEPATH += $$R2_INCLUDEPATH

    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    equals(USE_PKGCONFIG, 1) {
        CONFIG += link_pkgconfig
        PKGCONFIG += r_core
        R2_INCLUDEPATH = "$$system("pkg-config --variable=includedir r_core")/libr"
    } else {
        LIBS += \
        -lr_core \
        -lr_config \
        -lr_cons \
        -lr_io \
        -lr_flag \
        -lr_asm \
        -lr_debug \
        -lr_hash \
        -lr_bin \
        -lr_lang \
        -lr_parse \
        -lr_bp \
        -lr_egg \
        -lr_reg \
        -lr_search \
        -lr_syscall \
        -lr_socket \
        -lr_fs \
        -lr_anal \
        -lr_magic \
        -lr_util \
        -lr_crypto
    }
}
