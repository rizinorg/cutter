#include "CrashHandler.h"

#ifdef CUTTER_ENABLE_CRASH_REPORTS
#include "BugReporting.h"

#include <QStandardPaths>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <signal.h>
#include <QString>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QMap>

#if defined (Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined (Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#elif defined (Q_OS_MACOS)
#include "client/mac/handler/exception_handler.h"
#endif // Q_OS



// Here will be placed crash dump at the first place
// and then moved if needed
#if defined (Q_OS_LINUX) || defined (Q_OS_MACOS)
static std::string tmpLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString();
#else
static std::wstring tmpLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdWString();
#endif

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

static QString dumpFileFullPath = "";

#ifdef Q_OS_WIN32
// Called if crash dump was successfully created
// Saves path to file
bool callback(const wchar_t *_dump_dir,
              const wchar_t *_minidump_id,
              void *context, EXCEPTION_POINTERS *exinfo,
              MDRawAssertionInfo *assertion,
              bool success)
{
    QString dir = QString::fromWCharArray(_dump_dir);
    QString id = QString::fromWCharArray(_minidump_id);
    dumpFileFullPath = QDir(dir).filePath(id + ".dmp");
    return true;
}
#elif defined (Q_OS_LINUX)
// Called if crash dump was successfully created
// Saves path to file
bool callback(const google_breakpad::MinidumpDescriptor &md, void *context, bool b)
{
    dumpFileFullPath = md.path();
    return true;
}
#elif defined (Q_OS_MACOS)
// Called if crash dump was successfully created
// Saves path to file
bool callback(const char *dump_dir, const char *minidump_id, void *context, bool succeeded)
{
    QString dir = QString::fromUtf8(dump_dir);
    QString id = QString::fromUtf8(minidump_id);
    dumpFileFullPath = QDir(dir).filePath(id + ".dmp");
    return true;
}
#endif // Q_OS


/**
 * @brief Writes minidump and put its name in dumpFileFullPath.
 * @return true on succes
 */
bool writeMinidump()
{
    bool ok;
#if defined (Q_OS_LINUX) || defined (Q_OS_MACOS)
    ok = google_breakpad::ExceptionHandler::WriteMinidump(tmpLocation,
                                                          callback,
                                                          nullptr);
#elif defined (Q_OS_WIN32)
    ok = google_breakpad::ExceptionHandler::WriteMinidump(tmpLocation,
                                                          callback,
                                                          nullptr);
#endif // Q_OS
    return ok;
}

[[noreturn]] void crashHandler(int signum)
{
    // As soon as Cutter crashed, crash dump is created, so core and memory state
    // is not changed by all stuff with user interation going on below.
    bool ok = writeMinidump();

    QString err = sigNumDescription.contains(signum) ?
                      sigNumDescription[signum] :
                      QObject::tr("undefined");


    QMessageBox mb;
    mb.setWindowTitle(QObject::tr("Cutter encountered a problem"));
    mb.setText(QObject::tr("Cutter received a <b>%1</b> it can't handle and will close.<br/>"
                           "Would you like to create a crash dump for bug report?"
                           ).arg(err));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(QObject::tr("Create a crash dump"));
    mb.button(QMessageBox::No)->setText(QObject::tr("Do not report"));
    mb.setDefaultButton(QMessageBox::Yes);

    int ret = mb.exec();
    if (ret == QMessageBox::Yes) {
        QString dumpSaveFileName;
        int placementFailCounter = 0;
        do {
            placementFailCounter++;
            if (placementFailCounter == 4) {
                ok = false;
                break;
            }
            dumpSaveFileName = QFileDialog::getSaveFileName(nullptr,
                                                            QObject::tr("Choose a directory to save the crash dump in"),
                                                            QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                                                            QDir::separator() +
                                                            "Cutter_crash_dump_"
                                                            + QDate().currentDate().toString("dd.MM.yy") + "_"
                                                            + QTime().currentTime().toString() + ".dmp",
                                                            QObject::tr("Dump files (*.dmp)"));

            if (dumpSaveFileName.isEmpty()) {
                exit(3);
            }
            if (QFile::rename(dumpFileFullPath, dumpSaveFileName)) {
                break;
            }
            QMessageBox::critical(nullptr,
                                  QObject::tr("Error"),
                                  QObject::tr("Error occured during writing to the %1.<br/>"
                                              "Please, make sure you have access to that directory "
                                              "and try again.").arg(QFileInfo(dumpSaveFileName).dir().path()));
        } while (true);

        if (ok) {
            QMessageBox info;
            info.setWindowTitle(QObject::tr("Success"));
            info.setText(QObject::tr("<a href=\"%1\">Crash dump</a> was successfully created.")
                         .arg(QFileInfo(dumpSaveFileName).dir().path()));
            info.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

            info.button(QMessageBox::Yes)->setText(QObject::tr("Open an issue"));
            info.button(QMessageBox::No)->setText(QObject::tr("Exit Cutter"));
            info.setDefaultButton(QMessageBox::Yes);

            int ret = info.exec();
            if (ret == QMessageBox::Yes) {
                openIssue();
            }
        } else {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Error!"),
                                  QObject::tr("Error occured during crash dump creation."));
        }
    } else {
        QFile f(dumpFileFullPath);
        f.remove();
    }

    exit(3);
}

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

#else // CUTTER_ENABLE_CRASH_REPORTS

void initCrashHandler()
{

}

#endif // CUTTER_ENABLE_CRASH_REPORTS
