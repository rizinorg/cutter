#include "CutterApplication.h"

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

#ifdef CUTTER_ENABLE_JUPYTER
#include "utils/JupyterConnection.h"
#endif
#include "plugins/CutterPlugin.h"

CutterApplication::CutterApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    setOrganizationName("Cutter");
    setApplicationName("Cutter");
    setApplicationVersion(APP_VERSION);
    setWindowIcon(QIcon(":/img/cutter.svg"));
    setAttribute(Qt::AA_DontShowIconsInMenus);

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

#ifdef CUTTER_ENABLE_JUPYTER
    QCommandLineOption pythonHomeOption("pythonhome", QObject::tr("PYTHONHOME to use for Jupyter"),
                                        "PYTHONHOME");
    cmd_parser.addOption(pythonHomeOption);
#endif

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

#ifdef CUTTER_ENABLE_JUPYTER
    if (cmd_parser.isSet(pythonHomeOption)) {
        Jupyter()->setPythonHome(cmd_parser.value(pythonHomeOption));
    }
#endif

    bool analLevelSpecified = false;
    int analLevel = 0;

    // Initialize CutterCore and set default settings
    Core()->setSettings();

    if (cmd_parser.isSet(analOption)) {
        analLevel = cmd_parser.value(analOption).toInt(&analLevelSpecified);

        if (!analLevelSpecified || analLevel < 0 || analLevel > 2) {
            printf("%s\n",
                   QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.").toLocal8Bit().constData());
            exit(1);
        }
    }

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

    // Load plugins
    loadPlugins();
}

CutterApplication::~CutterApplication()
{
    delete mainWindow;
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
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            CutterPlugin *cutterPlugin = qobject_cast<CutterPlugin *>(plugin);
            if (cutterPlugin) {
                cutterPlugin->setupPlugin(Core());
                plugins.append(cutterPlugin);
            }
        }
    }

    Core()->setCutterPlugins(plugins);
}
