/** \file CutterDescriptions.h
 * This file contains every structure description that are used in widgets.
 * The descriptions are used for the Qt metatypes.
 */
#ifndef DESCRIPTIONS_H
#define DESCRIPTIONS_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QMetaType>
#include "core/CutterCommon.h"

struct FunctionDescription {
    RVA offset;
    RVA size;
    RVA nargs;
    RVA nbbs;
    RVA nlocals;
    QString calltype;
    QString name;
    RVA edges;
    RVA stackframe;

    bool contains(RVA addr) const
    {
        return addr >= offset && addr < offset + size;
    }
};

struct ImportDescription {
    RVA plt;
    int ordinal;
    QString bind;
    QString type;
    QString name;
};

struct ExportDescription {
    RVA vaddr;
    RVA paddr;
    RVA size;
    QString type;
    QString name;
    QString flag_name;
};

struct HeaderDescription
{
    RVA vaddr;
    RVA paddr;
    QString value;
    QString name;
};

struct ZignatureDescription
{
    QString name;
    QString bytes;
    RVA cc;
    RVA nbbs;
    RVA edges;
    RVA ebbs;
    RVA offset;
    QStringList refs;
};

struct TypeDescription {
    QString type;
    int size;
    QString format;
    QString category;
};

struct SearchDescription {
    RVA offset;
    int size;
    QString code;
    QString data;
};

struct SymbolDescription {
    RVA vaddr;
    QString bind;
    QString type;
    QString name;
};

struct CommentDescription {
    RVA offset;
    QString name;
};

struct RelocDescription {
    RVA vaddr;
    RVA paddr;
    QString type;
    QString name;
};

struct StringDescription {
    RVA vaddr;
    QString string;
    QString type;
    QString section;
    ut32 length;
    ut32 size;
};

struct FlagspaceDescription {
    QString name;
};

struct FlagDescription {
    RVA offset;
    RVA size;
    QString name;
};

struct SectionDescription {
    RVA vaddr;
    RVA paddr;
    RVA size;
    RVA vsize;
    QString name;
    QString perm;
    QString entropy;
};

struct SegmentDescription {
    RVA vaddr;
    RVA paddr;
    RVA size;
    RVA vsize;
    QString name;
    QString perm;
};

struct EntrypointDescription {
    RVA vaddr;
    RVA paddr;
    RVA baddr;
    RVA laddr;
    RVA haddr;
    QString type;
};

struct XrefDescription {
    RVA from;
    QString from_str;
    RVA to;
    QString to_str;
    QString type;
};

struct RBinPluginDescription {
    QString name;
    QString description;
    QString license;
    QString type;
};

struct RIOPluginDescription {
    QString name;
    QString description;
    QString license;
    QString permissions;
    QList<QString> uris;
};

struct RCorePluginDescription {
    QString name;
    QString description;
};

struct RAsmPluginDescription {
    QString name;
    QString architecture;
    QString author;
    QString version;
    QString cpus;
    QString description;
    QString license;
};

struct DisassemblyLine {
    RVA offset;
    QString text;
    RVA arrow;
};

struct BinClassBaseClassDescription {
    QString name;
    RVA offset;
};

struct BinClassMethodDescription {
    QString name;
    RVA addr = RVA_INVALID;
    st64 vtableOffset = -1;
};

struct BinClassFieldDescription {
    QString name;
    RVA addr = RVA_INVALID;
};

struct BinClassDescription {
    QString name;
    RVA addr = RVA_INVALID;
    RVA vtableAddr = RVA_INVALID;
    ut64 index = 0;
    QList<BinClassBaseClassDescription> baseClasses;
    QList<BinClassMethodDescription> methods;
    QList<BinClassFieldDescription> fields;
};

struct AnalMethodDescription {
    QString name;
    RVA addr;
    st64 vtableOffset;
};

struct AnalBaseClassDescription {
    QString id;
    RVA offset;
    QString className;
};

struct AnalVTableDescription {
    QString id;
    ut64 offset;
    ut64 addr;
};

struct ResourcesDescription {
    int name;
    RVA vaddr;
    ut64 index;
    QString type;
    ut64 size;
    QString lang;
};

struct VTableDescription {
    RVA addr;
    QList<BinClassMethodDescription> methods;
};

struct BlockDescription {
    RVA addr;
    RVA size;
    int flags;
    int functions;
    int inFunctions;
    int comments;
    int symbols;
    int strings;
    ut8 rwx;
};

struct BlockStatistics {
    RVA from;
    RVA to;
    RVA blocksize;
    QList<BlockDescription> blocks;
};

struct MemoryMapDescription {
    RVA addrStart;
    RVA addrEnd;
    QString name;
    QString fileName;
    QString type;
    QString permission;
};

struct BreakpointDescription {
    RVA addr;
    int size;
    QString permission;
    bool hw;
    bool trace;
    bool enabled;
};

struct ProcessDescription {
    int pid;
    int uid;
    QString status;
    QString path;
};

struct RegisterRefDescription {
    QString reg;
    QString value;
    QString ref;
};

struct VariableDescription {
    enum class RefType { SP, BP, Reg };
    RefType refType;
    QString name;
    QString type;
};

Q_DECLARE_METATYPE(FunctionDescription)
Q_DECLARE_METATYPE(ImportDescription)
Q_DECLARE_METATYPE(ExportDescription)
Q_DECLARE_METATYPE(SymbolDescription)
Q_DECLARE_METATYPE(CommentDescription)
Q_DECLARE_METATYPE(RelocDescription)
Q_DECLARE_METATYPE(StringDescription)
Q_DECLARE_METATYPE(FlagspaceDescription)
Q_DECLARE_METATYPE(FlagDescription)
Q_DECLARE_METATYPE(XrefDescription)
Q_DECLARE_METATYPE(EntrypointDescription)
Q_DECLARE_METATYPE(RBinPluginDescription)
Q_DECLARE_METATYPE(RIOPluginDescription)
Q_DECLARE_METATYPE(RCorePluginDescription)
Q_DECLARE_METATYPE(RAsmPluginDescription)
Q_DECLARE_METATYPE(BinClassMethodDescription)
Q_DECLARE_METATYPE(BinClassFieldDescription)
Q_DECLARE_METATYPE(BinClassDescription)
Q_DECLARE_METATYPE(const BinClassDescription *)
Q_DECLARE_METATYPE(const BinClassMethodDescription *)
Q_DECLARE_METATYPE(const BinClassFieldDescription *)
Q_DECLARE_METATYPE(AnalBaseClassDescription)
Q_DECLARE_METATYPE(AnalMethodDescription)
Q_DECLARE_METATYPE(AnalVTableDescription)
Q_DECLARE_METATYPE(ResourcesDescription)
Q_DECLARE_METATYPE(VTableDescription)
Q_DECLARE_METATYPE(TypeDescription)
Q_DECLARE_METATYPE(HeaderDescription)
Q_DECLARE_METATYPE(ZignatureDescription)
Q_DECLARE_METATYPE(SearchDescription)
Q_DECLARE_METATYPE(SectionDescription)
Q_DECLARE_METATYPE(SegmentDescription)
Q_DECLARE_METATYPE(MemoryMapDescription)
Q_DECLARE_METATYPE(BreakpointDescription)
Q_DECLARE_METATYPE(ProcessDescription)
Q_DECLARE_METATYPE(RegisterRefDescription)
Q_DECLARE_METATYPE(VariableDescription)

#endif // DESCRIPTIONS_H
