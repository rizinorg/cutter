win32 {
    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    INCLUDEPATH += "$$PWD/../iaito_win32/include"
    INCLUDEPATH += "$$PWD/../iaito_win32/radare2/include/libr"
    !contains(QT_ARCH, x86_64) {
        LIBS += -L"$$PWD/../iaito_win32/radare2/lib32"
    } else {
        LIBS += -L"$$PWD/../iaito_win32/radare2/lib64"
    }

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
        -lr_io \
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
    USE_PKGCONFIG = 1
    R2_USER_PKGCONFIG = $$(HOME)/bin/prefix/radare2/lib/pkgconfig
    exists($$R2_USER_PKGCONFIG) {
        # caution: may not work for cross compilations
        QMAKE_PKG_CONFIG = PKG_CONFIG_PATH=$$R2_USER_PKGCONFIG pkg-config
    } else {
        exists(/usr/local/lib/pkgconfig/r_core.pc) {
            QMAKE_PKG_CONFIG = PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config
            LIBS += -L/usr/local/lib
            INCLUDEPATH += /usr/local/include/libr
            USE_PKGCONFIG = 0
        }
    }

    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    USE_PKGCONFIG {
        CONFIG += link_pkgconfig
        PKGCONFIG += r_core
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
        -lr_io \
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
