#include "CutterApplication.h"

#include <QApplication>
#include <QFileOpenEvent>
#include <QEvent>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QStringList>
#include <QProcess>

CutterApplication::CutterApplication(int &argc, char **argv) : QApplication(argc, argv){
    setOrganizationName("Cutter");
    setApplicationName("Cutter");
    setApplicationVersion(APP_VERSION);
    setWindowIcon(QIcon(":/img/cutter.svg"));

    // Set QString codec to UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif
    QCommandLineParser cmd_parser;
    cmd_parser.setApplicationDescription(QObject::tr("A Qt and C++ GUI for radare2 reverse engineering framework"));
    cmd_parser.addHelpOption();
    cmd_parser.addVersionOption();
    cmd_parser.addPositionalArgument("filename", QObject::tr("Filename to open."));

    QCommandLineOption analOption({"A", "anal"},
                                   QObject::tr("Automatically open file and optionally start analysis. Needs filename to be specified. May be a value between 0 and 2: 0 = no analysis, 1 = aaa, 2 = aaaa (experimental)"),
                                   QObject::tr("level"));
    cmd_parser.addOption(analOption);

    cmd_parser.process(*this);

    QStringList args = cmd_parser.positionalArguments();

    // Check r2 version
    QString r2version = r_core_version();
    QString localVersion = "" R2_GITTAP;
    if (r2version != localVersion)
    {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setWindowTitle(QObject::tr("Version mismatch!"));
        msg.setText(QString(QObject::tr("The version used to compile Cutter (%1) does not match the binary version of radare2 (%2). This could result in unexpected behaviour. Are you sure you want to continue?")).arg(localVersion, r2version));
        if (msg.exec() == QMessageBox::No)
            exit(1);
    }

    bool analLevelSpecified = false;
    int analLevel= 0;

    if (cmd_parser.isSet(analOption))
    {
        analLevel = cmd_parser.value(analOption).toInt(&analLevelSpecified);

        if (!analLevelSpecified || analLevel < 0 || analLevel > 2)
        {
            printf("%s\n", QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.").toLocal8Bit().constData());
            exit(1);
        }
    }

    MainWindow *main = new MainWindow();

    setMainWindow(main);

    if (args.empty())
    {
        if (analLevelSpecified)
        {
            printf("%s\n", QObject::tr("Filename must be specified to start analysis automatically.").toLocal8Bit().constData());
            exit(1);
        }

        main->displayNewFileDialog();
    }
    else // filename specified as positional argument
    {
        main->openNewFile(args[0], analLevelSpecified ? analLevel : -1);
    }
}

bool CutterApplication::event(QEvent *e){
    if (e->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(e);
        if (openEvent) {
            if (m_FileAlreadyDropped) {
                // we already dropped a file in macOS, let's spawn another instance
                // (Like the File -> Open)
                QString fileName = openEvent->file();
                QProcess process(this);
                process.setEnvironment(QProcess::systemEnvironment());
                QStringList args = QStringList(fileName);
                process.startDetached(qApp->applicationFilePath(), args);
            } else {
                QString fileName = openEvent->file();
                m_FileAlreadyDropped = true;
                m_MainWindow->closeNewFileDialog();
                m_MainWindow->openNewFile(fileName, -1);
            }
        }
    }
    return QApplication::event(e);
}
