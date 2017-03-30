#include "qrdisasm.h"

// This class is not used, created by pancake a long time ago.
// Kept here just because

QRDisasm::QRDisasm(QRCore *core)
{
    this->core = core;
}

bool QRDisasm::disassembleAt (ut64 addr, QRDisasmOption opt, QRDisasmRow &dr) {
    printf ("FUCK\n");
    return false;
}
