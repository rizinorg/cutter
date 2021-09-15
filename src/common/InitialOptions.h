
#ifndef CUTTER_INITIALOPTIONS_H
#define CUTTER_INITIALOPTIONS_H

#include "core/Cutter.h"

/**
 * @brief The CommandDescription struct is a pair of a Rizin command and its description
 */
struct CommandDescription
{
    QString command;
    QString description;
};

struct InitialOptions
{
    enum class Endianness { Auto, Little, Big };

    QString filename;
    QString projectFile;

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

    QList<CommandDescription> analysisCmd = { { "aaa", "Auto analysis" } };

    QString shellcode;
};

#endif // CUTTER_INITIALOPTIONS_H
