win32 {
    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    INCLUDEPATH += "$$PWD/../iaito_win32/include"
    INCLUDEPATH += "$$PWD/../iaito_win32/radare2/include/libr"
    !contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L"$$PWD/../iaito_win32/radare2/lib32"
    } else {
        LIBS += -L"$$PWD/../iaito_win32/radare2/lib64"
    }
} else {
    # check if r2 is available
    system(r2 > /dev/null 2>&1) {

        # see https://github.com/hteso/iaito/pull/5#issuecomment-290433796
        RADARE2_INCLUDE_PATH = $$system(r2 -H | grep INCDIR | sed 's/[^=]*=//')
        RADARE2_LIB_PATH = $$system(r2 -H | grep LIBDIR | sed 's/[^=]*=//')

        !isEmpty(RADARE2_INCLUDE_PATH) {
            INCLUDEPATH *= $$RADARE2_INCLUDE_PATH
            LIBS *= -L$$RADARE2_LIB_PATH
        } else {
            error("sorry could not find radare2 lib")
        }
    } else {
        error("r2 not found/in path")
    }
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

