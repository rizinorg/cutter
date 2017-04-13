#ifndef QRCORE_H
#define QRCORE_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

//Workaround for compile errors on Windows
#ifdef _WIN32
#include <r2hacks.h>
#endif //_WIN32

#include "r_core.h"

//Workaround for compile errors on Windows.
#ifdef _WIN32
#undef min
#endif //_WIN32

#define HAVE_LATEST_LIBR2 false

#define QRListForeach(list, it, type, x) \
    if (list) for (it = list->head; it && ((x=(type*)it->data)); it = it->n)

#define __alert(x) QMessageBox::question (this, "Alert", QString(x), QMessageBox::Ok)
#define __question(x) (QMessageBox::Yes==QMessageBox::question (this, "Alert", QString(x), QMessageBox::Yes| QMessageBox::No))

class RCoreLocked
{
    RCore *core;

public:
    explicit RCoreLocked(RCore *core);
    RCoreLocked(const RCoreLocked &) = delete;
    RCoreLocked &operator=(const RCoreLocked &) = delete;
    RCoreLocked(RCoreLocked &&);
    ~RCoreLocked();
    operator RCore *() const;
    RCore *operator->() const;
};

#define QNOTUSED(x) do { (void)(x); } while ( 0 );

class QRCore : public QObject
{
    Q_OBJECT
public:
    QString projectPath;
    explicit QRCore(QObject *parent = 0);
    ~QRCore();
    QList<QString> getFunctionXrefs(ut64 addr);
    QList<QString> getFunctionRefs(ut64 addr, char type);
    int getCycloComplex(ut64 addr);
    int getFcnSize(ut64 addr);
    int fcnCyclomaticComplexity(ut64 addr);
    int fcnBasicBlockCount(ut64 addr);
    int fcnEndBbs(QString addr);
    QString cmd(const QString &str);
    QJsonDocument cmdj(const QString &str);
    void renameFunction(QString prev_name, QString new_name);
    void setComment(QString addr, QString cmt);
    void delComment(ut64 addr);
    QList<QList<QString>> getComments();
    QMap<QString, QList<QList<QString>>> getNestedComments();
    void setOptions(QString key);
    bool loadFile(QString path, uint64_t loadaddr = 0LL, uint64_t mapaddr = 0LL, bool rw = false, int va = 0, int bits = 0, int idx = 0, bool loadbin = false);
    bool tryFile(QString path, bool rw);
    void analyze(int level);
    void seek(QString addr);
    void seek(ut64 addr);
    ut64 math(const QString &expr);
    QString itoa(ut64 num, int rdx = 16);
    QString config(const QString &k, const QString &v = NULL);
    int config(const QString &k, int v);
    QList<QString> getList(const QString &type, const QString &subtype = "");
    QString assemble(const QString &code);
    QString disassemble(const QString &hex);
    void setDefaultCPU();
    void setCPU(QString arch, QString cpu, int bits, bool temporary = false);
    RAnalFunction *functionAt(ut64 addr);
    QString cmdFunctionAt(QString addr);
    /* sdb */
    QList<QString> sdbList(QString path);
    QList<QString> sdbListKeys(QString path);
    QString sdbGet(QString path, QString key);
    bool sdbSet(QString path, QString key, QString val);
    int get_size();
    ulong get_baddr();
    QList<QList<QString>> get_exec_sections();
    QString getOffsetInfo(QString addr);
    QString getOffsetJump(QString addr);
    QString getDecompiledCode(QString addr);
    QString getFileInfo();
    QStringList getStats();
    QString getSimpleGraph(QString function);
    QString binStart;
    QString binEnd;
    void getOpcodes();
    QList<QString> opcodes;
    QList<QString> regs;
    void setSettings();

    RCoreLocked core() const;

    /* fields */

    Sdb *db;
signals:

public slots:

private:
    QString default_arch;
    QString default_cpu;
    int default_bits;

    RCore *core_;
};

#endif // QRCORE_H
