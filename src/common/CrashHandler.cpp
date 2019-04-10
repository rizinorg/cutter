#include "CrashHandler.h"
#include "BugReporting.h"

#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>

#ifdef CUTTER_ENABLE_CRASH_REPORTS

#include <QApplication>
#include <QString>
#include <QFile>
#include <QDir>
#include <QMap>
#include <QProcess>

#if defined (Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined (Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#elif defined (Q_OS_MACOS)
#include "client/mac/handler/exception_handler.h"
#endif // Q_OS

static google_breakpad::ExceptionHandler *exceptionHandler = nullptr;

static void finishCrashHandler()
{
    delete exceptionHandler;
}

#ifdef Q_OS_WIN32
// Called if crash dump was successfully created
// Saves path to file
bool callback(const wchar_t *_dump_dir,
              const wchar_t *_minidump_id,
              void *context, EXCEPTION_POINTERS *exinfo,
              MDRawAssertionInfo *assertion,
              bool success)
{
    const QDir dir = QString::fromWCharArray(_dump_dir);
    const QString id = QString::fromWCharArray(_minidump_id);
    QProcess::startDetached(QCoreApplication::applicationFilePath(),
        { "--start-crash-handler", dir.filePath(id + ".dmp") });
    _exit(1);
    return true;
}
#elif defined (Q_OS_LINUX)
// Called if crash dump was successfully created
// Saves path to file
bool callback(const google_breakpad::MinidumpDescriptor &md, void *context, bool b)
{
    QProcess::startDetached(QCoreApplication::applicationFilePath(),
        { "--start-crash-handler", md.path() });
    _exit(1);
    return true;
}
#elif defined (Q_OS_MACOS)
// Called if crash dump was successfully created
// Saves path to file
bool callback(const char *dump_dir, const char *minidump_id, void *context, bool succeeded)
{
    const QDir dir = QString::fromUtf8(dump_dir);
    const QString id = QString::fromUtf8(minidump_id);
    QProcess::startDetached(QCoreApplication::applicationFilePath(),
        { "--start-crash-handler", dir.filePath(id + ".dmp") });
    _exit(1);
    return true;
}
#endif // Q_OS

void initCrashHandler()
{
    if (exceptionHandler) {
        return;
    }
    // Here will be placed crash dump at the first place
    // and then moved if needed

#if defined (Q_OS_LINUX)
    static std::string tmpLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString();
    exceptionHandler = new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor(tmpLocation),
                                                             nullptr,
                                                             callback,
                                                             nullptr,
                                                             true,
                                                             -1);
#elif defined (Q_OS_MACOS)
    static std::string tmpLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString();
    exceptionHandler = new google_breakpad::ExceptionHandler(tmpLocation,
                                                             nullptr,
                                                             callback,
                                                             nullptr,
                                                             true,
                                                             nullptr);
#else
    static std::wstring tmpLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdWString();
    exceptionHandler = new google_breakpad::ExceptionHandler(tmpLocation,
                                                             nullptr,
                                                             callback,
                                                             nullptr,
                                                             google_breakpad::ExceptionHandler::HANDLER_ALL);
#endif
    atexit(finishCrashHandler);
}

#else // CUTTER_ENABLE_CRASH_REPORTS

void initCrashHandler()
{

}

#endif // CUTTER_ENABLE_CRASH_REPORTS


void showCrashDialog(const QString &dumpFile)
{
    QMessageBox mb;
    mb.setWindowTitle(QObject::tr("Crash"));
    mb.setText(QObject::tr("Cutter received a signal it can't handle and will close.<br/>"
                           "Would you like to create a crash dump for a bug report?"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.button(QMessageBox::Yes)->setText(QObject::tr("Create a Crash Dump"));
    mb.button(QMessageBox::No)->setText(QObject::tr("Quit"));
    mb.setDefaultButton(QMessageBox::Yes);

    bool ok = false;
    int ret = mb.exec();
    if (ret == QMessageBox::Yes) {
        QString dumpSaveFileName;
        int placementFailCounter = 0;
        do {
            placementFailCounter++;
            if (placementFailCounter == 4) {
                break;
            }
            dumpSaveFileName = QFileDialog::getSaveFileName(nullptr,
                                                            QObject::tr("Choose a directory to save the crash dump in"),
                                                            QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                                                            QDir::separator() +
                                                            "Cutter_crash_dump_"
                                                            + QDate::currentDate().toString("dd.MM.yy") + "_"
                                                            + QTime::currentTime().toString("HH.mm.ss") + ".dmp",
                                                            QObject::tr("Minidump (*.dmp)"));

            if (dumpSaveFileName.isEmpty()) {
                return;
            }
            if (QFile::rename(dumpFile, dumpSaveFileName)) {
                ok = true;
                break;
            }
            QMessageBox::critical(nullptr,
                                  QObject::tr("Save Crash Dump"),
                                  QObject::tr("Failed to write to %1.<br/>"
                                              "Please make sure you have access to that directory "
                                              "and try again.").arg(QFileInfo(dumpSaveFileName).dir().path()));
        } while (true);

        if (ok) {
            QMessageBox info;
            info.setWindowTitle(QObject::tr("Success"));
            info.setText(QObject::tr("<a href=\"%1\">Crash dump</a> was successfully created.")
                         .arg(QFileInfo(dumpSaveFileName).dir().path()));
            info.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

            info.button(QMessageBox::Yes)->setText(QObject::tr("Open an Issue"));
            info.button(QMessageBox::No)->setText(QObject::tr("Quit"));
            info.setDefaultButton(QMessageBox::Yes);

            int ret = info.exec();
            if (ret == QMessageBox::Yes) {
                openIssue();
            }
        } else {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Error"),
                                  QObject::tr("Error occurred during crash dump creation."));
        }
    } else {
        QFile f(dumpFile);
        f.remove();
    }
}
