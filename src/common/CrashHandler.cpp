#include "CrashHandler.h"
#include <QStandardPaths>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <signal.h>
#include <QString>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QMap>

#ifdef CUTTER_ENABLE_CRASH_REPORTS
#if defined (Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined (Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#elif defined (Q_OS_MACOS)
#include "client/mac/handler/exception_handler.h"
#endif // Q_OS
#endif // CUTTER_ENABLE_CRASH_REPORTS

#include "Cutter.h"

static const QMap<int, QString> sigNumDescription = {
    #ifdef SIGSEGV
    { SIGSEGV, "SIGSEGV" },
    #endif // SIGSEGV
    #ifdef SIGILL
    { SIGILL, "SIGILL" },
    #endif // SIGILL
    #ifdef SIGFPE
    { SIGFPE, "SIGFPE" },
    #endif // SIGFPE
    #ifdef SIGABRT
    { SIGABRT, "SIGABRT" },
    #endif // SIGABRT
    #ifdef SIGBUS
    { SIGBUS, "SIGBUS" },
    #endif // SIGBUS
    #ifdef SIGPIPE
    { SIGPIPE, "SIGPIPE" },
    #endif // SIGPIPE
    #ifdef SIGSYS
    { SIGSYS, "SIGSYS" }
    #endif // SIGSYS
};

#ifdef CUTTER_ENABLE_CRASH_REPORTS

static QString dumpFileFullPath = "";

#ifdef Q_OS_WIN32
bool callback(const wchar_t *_dump_dir,
              const wchar_t *_minidump_id,
              void *context, EXCEPTION_POINTERS *exinfo,
              MDRawAssertionInfo *assertion,
              bool success)
{
    QString dir = QString::fromWCharArray(_dump_dir);
    QString id = QString::fromWCharArray(_minidump_id);
    return QFile::rename(QDir(dir).filePath(id + ".dmp"), dumpFileFullPath);
}
#elif defined (Q_OS_LINUX)
bool callback(const google_breakpad::MinidumpDescriptor &md, void *context, bool b)
{
    return QFile::rename(md.path(), dumpFileFullPath);
}
#elif defined (Q_OS_MACOS)
bool callback(const char *dump_dir, const char *minidump_id, void *context, bool succeeded)
{
    QString dir = QString::fromUtf8(dump_dir);
    QString id = QString::fromUtf8(minidump_id);
    return QFile::rename(QDir(dir).filePath(id + ".dmp"), dumpFileFullPath);
}
#endif // Q_OS

[[noreturn]] void crashHandler(int signum)
{
    QString err = sigNumDescription.contains(signum) ?
                      sigNumDescription[signum] :
                      QObject::tr("undefined");


    QMessageBox mb;
    mb.setWindowTitle(QObject::tr("Cutter encountered a problem"));
    mb.setText(QObject::tr("Cutter got a <b>%1</b> signal and can't handle it, so "
                           "programm will close.<br/>"
                           "Would you like to create a crash dump for bug report?"
                           ).arg(err));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(QObject::tr("Create a crash dump"));
    mb.button(QMessageBox::No)->setText(QObject::tr("Do not report"));

    int ret = mb.exec();
    if (ret == QMessageBox::Yes) {
        QString dir =
                QFileDialog::getExistingDirectory(nullptr,
                                                  QObject::tr("Choose a directory to save the crash dump in"),
                                                  QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        dumpFileFullPath = QDir(dir).filePath("Cutter_crash_dump_" + QDate().currentDate().toString("dd.MM.yy") + "_"
                                              + QTime().currentTime().toString() + ".dmp");
        bool ok;
#if defined (Q_OS_LINUX) || defined (Q_OS_MACOS)
        ok = google_breakpad::ExceptionHandler::WriteMinidump(dir.toStdString(),
                                                              callback,
                                                              nullptr);
#elif defined (Q_OS_WIN32)
        ok = google_breakpad::ExceptionHandler::WriteMinidump(dir.toStdWString(),
                                                              callback,
                                                              nullptr);
#endif // Q_OS
        if (ok) {
            QMessageBox info;
            info.setWindowTitle(QObject::tr("Success"));
            info.setText(QObject::tr("<a href=\"%1\">Crash dump</a> was successfully created.").arg(dir));
            info.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

            info.button(QMessageBox::Yes)->setText(QObject::tr("Open an issue"));
            info.button(QMessageBox::No)->setText(QObject::tr("Exit Cutter"));

            int ret = info.exec();
            if (ret == QMessageBox::Yes) {
                Core()->openIssue();
            }
        } else {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Error!"),
                                  QObject::tr("Error occured during crash dump creation."));
        }
    }

    exit(3);
}
#else
[[noreturn]] void crashHandler(int signum)
{
    QString err = sigNumDescription.contains(signum) ?
                      sigNumDescription[signum] :
                      QObject::tr("undefined");

    QMessageBox mb;
    mb.setWindowTitle(QObject::tr("Cutter encountered a problem"));
    mb.setText(QObject::tr("Cutter got a <b>%1</b> signal and can't handle it, so "
                           "programm will close.<br/>"
                           "Would you like to create an issue for bug report?"
                           ).arg(err));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(QObject::tr("Open an issue"));
    mb.button(QMessageBox::No)->setText(QObject::tr("Do not report"));

    int ret = mb.exec();
    if (ret == QMessageBox::Yes) {
        Core()->openIssue();
    }
    exit(3);
}
#endif // CUTTER_ENABLE_CRASH_REPORTS

void initCrashHandler()
{
#ifdef SIGSEGV
    signal(SIGSEGV, crashHandler);
#endif // SIGSEGV
#ifdef SIGILL
    signal(SIGILL, crashHandler);
#endif // SIGILL
#ifdef SIGFPE
    signal(SIGFPE, crashHandler);
#endif // SIGFPE
#ifdef SIGABRT
    signal(SIGABRT, crashHandler);
#endif // SIGABRT
#ifdef SIGBUS
    signal(SIGBUS, crashHandler);
#endif // SIGBUS
#ifdef SIGPIPE
    signal(SIGPIPE, crashHandler);
#endif // SIGPIPE
#ifdef SIGSYS
    signal(SIGSYS, crashHandler);
#endif // SIGSYS
}
