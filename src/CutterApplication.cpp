#include "utils/PythonManager.h"
#include "CutterApplication.h"
#ifdef CUTTER_ENABLE_JUPYTER
#include "utils/JupyterConnection.h"
#endif

#include <QApplication>
#include <QFileOpenEvent>
#include <QEvent>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QStringList>
#include <QProcess>
#include <QPluginLoader>
#include <QDir>

CutterApplication::CutterApplication(int &argc, char **argv) : QApplication(argc, argv)
{
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
    cmd_parser.setApplicationDescription(
        QObject::tr("A Qt and C++ GUI for radare2 reverse engineering framework"));
    cmd_parser.addHelpOption();
    cmd_parser.addVersionOption();
    cmd_parser.addPositionalArgument("filename", QObject::tr("Filename to open."));

    QCommandLineOption analOption({"A", "anal"},
                                  QObject::tr("Automatically open file and optionally start analysis. Needs filename to be specified. May be a value between 0 and 2: 0 = no analysis, 1 = aaa, 2 = aaaa (experimental)"),
                                  QObject::tr("level"));
    cmd_parser.addOption(analOption);

    QCommandLineOption pythonHomeOption("pythonhome", QObject::tr("PYTHONHOME to use for embeded python interpreter"),
                                        "PYTHONHOME");
    cmd_parser.addOption(pythonHomeOption);

    cmd_parser.process(*this);

    QStringList args = cmd_parser.positionalArguments();

    // Check r2 version
    QString r2version = r_core_version();
    QString localVersion = "" R2_GITTAP;
    if (r2version != localVersion) {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setWindowTitle(QObject::tr("Version mismatch!"));
        msg.setText(QString(
                        QObject::tr("The version used to compile Cutter (%1) does not match the binary version of radare2 (%2). This could result in unexpected behaviour. Are you sure you want to continue?")).arg(
                        localVersion, r2version));
        if (msg.exec() == QMessageBox::No)
            exit(1);
    }

    // Init python
    if (cmd_parser.isSet(pythonHomeOption)) {
        Python()->setPythonHome(cmd_parser.value(pythonHomeOption));
    }
    Python()->initialize();


    bool analLevelSpecified = false;
    int analLevel = 0;

    if (cmd_parser.isSet(analOption)) {
        analLevel = cmd_parser.value(analOption).toInt(&analLevelSpecified);

        if (!analLevelSpecified || analLevel < 0 || analLevel > 2) {
            printf("%s\n",
                   QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.").toLocal8Bit().constData());
            exit(1);
        }
    }

    loadPlugins();

    mainWindow = new MainWindow();
    installEventFilter(mainWindow);

    if (args.empty()) {
        if (analLevelSpecified) {
            printf("%s\n",
                   QObject::tr("Filename must be specified to start analysis automatically.").toLocal8Bit().constData());
            exit(1);
        }

        mainWindow->displayNewFileDialog();
    } else { // filename specified as positional argument
        mainWindow->openNewFile(args[0], analLevelSpecified ? analLevel : -1);
    }
}

CutterApplication::~CutterApplication()
{
    delete mainWindow;
    delete Python();
}

bool CutterApplication::event(QEvent *e)
{
    if (e->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(e);
        if (openEvent) {
            if (m_FileAlreadyDropped) {
                // We already dropped a file in macOS, let's spawn another instance
                // (Like the File -> Open)
                QString fileName = openEvent->file();
                QProcess process(this);
                process.setEnvironment(QProcess::systemEnvironment());
                QStringList args = QStringList(fileName);
                process.startDetached(qApp->applicationFilePath(), args);
            } else {
                QString fileName = openEvent->file();
                m_FileAlreadyDropped = true;
                mainWindow->closeNewFileDialog();
                mainWindow->openNewFile(fileName, -1);
            }
        }
    }
    return QApplication::event(e);
}

void CutterApplication::loadPlugins()
{
    QList<CutterPlugin*> plugins;
    QDir pluginsDir(qApp->applicationDirPath());
    #if defined(Q_OS_WIN)
        if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
            pluginsDir.cdUp();
    #elif defined(Q_OS_MAC)
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
    #endif
    pluginsDir.cd("plugins");
    Python()->addPythonPath(pluginsDir.absolutePath().toLatin1().data());

    CutterPlugin *cutterPlugin = nullptr;
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        if (fileName.endsWith(".py")) {
            // Load python plugins
            QStringList l = fileName.split(".py");
            cutterPlugin = (CutterPlugin*) Python()->loadPlugin(l.at(0).toLatin1().constData());
        } else {
            // Load C++ plugins
            QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
            QObject *plugin = pluginLoader.instance();
            if (plugin) {
                cutterPlugin = qobject_cast<CutterPlugin *>(plugin);
            }
        }

        if (cutterPlugin) {
            cutterPlugin->setupPlugin(Core());
            plugins.append(cutterPlugin);
        }
    }

    qInfo() << "Loaded" << plugins.length() << "plugins.";
    Core()->setCutterPlugins(plugins);
}
