#ifndef DECOMPILER_H
#define DECOMPILER_H

#include "CutterCommon.h"

#include <QString>
#include <QObject>

/**
 * Describes the result of a Decompilation Process with optional metadata
 */
struct DecompiledCode {
    /**
     * A single line of decompiled code
     */
    struct Line
    {
        QString str;

        /**
         * Offset of the original instruction
         */
        RVA addr;

        Line()
        {
            this->addr = RVA_INVALID;
        }

        explicit Line(const QString &str, RVA addr = RVA_INVALID)
        {
            this->str = str;
            this->addr = addr;
        }
    };
    QList<Line> lines = {};
};

/**
 * Implements a decompiler that can be registered using CutterCore::registerDecompiler()
 */
class Decompiler: public QObject
{
    Q_OBJECT

private:
    const QString id;
    const QString name;

public:
    Decompiler(const QString &id, const QString &name, QObject *parent = nullptr);
    virtual ~Decompiler() = default;

    QString getId() const   { return id; }
    QString getName() const { return name; }

    virtual DecompiledCode decompileAt(RVA addr) =0;
};

class R2DecDecompiler: public Decompiler
{
    Q_OBJECT

public:
    explicit R2DecDecompiler(QObject *parent = nullptr);
    DecompiledCode decompileAt(RVA addr) override;

    static bool isAvailable();
};

#endif //DECOMPILER_H
