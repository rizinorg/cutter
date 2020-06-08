#ifndef DECOMPILER_H
#define DECOMPILER_H

#include "CutterCommon.h"
#include "R2Task.h"
#include <r_util/r_annotated_code.h>

#include <QString>
#include <QObject>

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

    static RAnnotatedCode *makeWarning(QString warningMessage);

    QString getId() const       { return id; }
    QString getName() const     { return name; }
    virtual bool isRunning()    { return false; }
    virtual bool isCancelable() { return false; }

    virtual void decompileAt(RVA addr) =0;
    virtual void cancel() {}

signals:
    void finished(RAnnotatedCode *codeDecompiled);
};

class R2DecDecompiler: public Decompiler
{
    Q_OBJECT

private:
    R2Task *task;

public:
    explicit R2DecDecompiler(QObject *parent = nullptr);
    void decompileAt(RVA addr) override;

    bool isRunning() override    { return task != nullptr; }

    static bool isAvailable();
};

#endif //DECOMPILER_H
