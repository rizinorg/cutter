#include "qrdisasm.h"

// This class is not used, created by pancake a long time ago.
// Kept here just because

QRDisasm::QRDisasm(QRCore *core) :
    core(core),
    db(nullptr)
{
}

bool QRDisasm::disassembleAt(ut64 addr, QRDisasmOption opt, QRDisasmRow &dr)
{
    QNOTUSED(addr);
    QNOTUSED(opt);
    QNOTUSED(dr);

    printf("FUCK\n");
    return false;
}
