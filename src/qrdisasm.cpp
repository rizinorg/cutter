#include "iaitordisasm.h"

// This class is not used, created by pancake a long time ago.
// Kept here just because

IaitoRDisasm::IaitoRDisasm(IaitoRCore *core) :
    core(core),
    db(nullptr)
{
}

bool IaitoRDisasm::disassembleAt(ut64 addr, IaitoRDisasmOption opt, IaitoRDisasmRow &dr)
{
    IAITONOTUSED(addr);
    IAITONOTUSED(opt);
    IAITONOTUSED(dr);

    printf("FUCK\n");
    return false;
}
