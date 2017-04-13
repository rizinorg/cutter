#ifndef QRDISASM_H
#define QRDISASM_H

#include <qrcore.h>

enum QRDisasmDataType
{
    STRING = 'z',
    STRUCT = 's',
    DATA = 'd',

};

enum QRDisasmOption
{
    DWARF = 1 << 1,
    REFS = 1 << 2,
    ESIL = 1 << 3,
    HEXPAIRS = 1 << 4,
    COMMENT = 1 << 5,
};

class QRDisasmRow
{
private:
    QRCore *core;
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
    QRDisasmDataType datatype;
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


class QRDisasm
{
    QRCore *core;
    Sdb *db;
public:
    explicit QRDisasm(QRCore *core);
    bool disassembleAt(ut64 addr, QRDisasmOption opt, QRDisasmRow &dr);
    // high level api for the disasm thing to manage comments, xrefs, etc
    //next();
    //prev();

};

#endif // QRDISASM_H
