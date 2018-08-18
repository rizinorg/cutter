
#ifndef CUTTER_INITIALOPTIONS_H
#define CUTTER_INITIALOPTIONS_H

#include "Cutter.h"

struct InitialOptions
{
    enum class Endianness { Auto, Little, Big };

    QString filename;

    bool useVA = true;
    RVA binLoadAddr = RVA_INVALID;
    RVA mapAddr = RVA_INVALID;

    QString arch;
    QString cpu;
    int bits = 0;
    QString os;

    Endianness endian;

    bool writeEnabled = false;
    bool loadBinInfo = true;
    QString forceBinPlugin;

    bool demangle = true;

    QString pdbFile;
    QString script;

    int bbsize = 0;

    QList<QString> analCmd = { "aaa" };

    QString shellcode;
};

#endif //CUTTER_INITIALOPTIONS_H
