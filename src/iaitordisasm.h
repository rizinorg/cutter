#ifndef IAITORDISASM_H
#define IAITORDISASM_H

#include <iaitorcore.h>

enum IaitoRDisasmDataType
{
    STRING = 'z',
    STRUCT = 's',
    DATA = 'd',

};

enum IaitoRDisasmOption
{
    DWARF = 1 << 1,
    REFS = 1 << 2,
    ESIL = 1 << 3,
    HEXPAIRS = 1 << 4,
    COMMENT = 1 << 5,
};

class IaitoRDisasmRow
{
private:
    IaitoRCore *core;
public:
    ut64 vaddr;
    ut64 paddr;
    int size;
    QString hex;

    // Context
    // list of flags
    // closer function
    RFlagItem *flag;
    RAnalFunction *fcn;

    // Contents

    // if instruction
    int optype; // jmp, cjmp, ... // aka RAnalOp
    QString mnemonic;
    QString arg[3]; // each argument splitted here
    QString disasm;
    QString esil;

    // if data
    IaitoRDisasmDataType datatype;
    QString dataopt; // struct name, aliased name, string, etc
    // data type
    // string/struct/hex/word

    // Comment
    QString comment;
    QString description;
    // References
    // refs
    // xrefs
};


class IaitoRDisasm
{
    IaitoRCore *core;
    Sdb *db;
public:
    explicit IaitoRDisasm(IaitoRCore *core);
    bool disassembleAt(ut64 addr, IaitoRDisasmOption opt, IaitoRDisasmRow &dr);
    // high level api for the disasm thing to manage comments, xrefs, etc
    //next();
    //prev();

};

#endif // IAITORDISASM_H
