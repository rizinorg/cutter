#include "common/PythonManager.h"
#include "CutterApplication.h"
#ifdef CUTTER_ENABLE_JUPYTER
#include "common/JupyterConnection.h"
#endif
#include "plugins/PluginManager.h"

#include <QApplication>
#include <QFileOpenEvent>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QStringList>
#include <QProcess>
#include <QPluginLoader>
#include <QDir>
#include <QTranslator>
#include <QLibraryInfo>

#include "CutterConfig.h"

#include <cstdlib>

CutterApplication::CutterApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    // Setup application information
    setApplicationVersion(CUTTER_VERSION_FULL);
    setWindowIcon(QIcon(":/img/cutter.svg"));
    setAttribute(Qt::AA_DontShowIconsInMenus);
    setLayoutDirection(Qt::LeftToRight);

    // WARN!!! Put initialization code below this line. Code above this line is mandatory to be run First
    // Load translations
    if (!loadTranslations()) {
        qWarning() << "Cannot load translations";
    }

    // Load fonts
    int ret = QFontDatabase::addApplicationFont(":/fonts/Anonymous Pro.ttf");
    if (ret == -1) {
        qWarning() << "Cannot load Anonymous Pro font.";
    }

    ret = QFontDatabase::addApplicationFont(":/fonts/Inconsolata-Regular.ttf");
    if (ret == -1) {
        qWarning() << "Cannot load Incosolata-Regular font.";
    }


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

    QCommandLineOption scriptOption("i",
                                    QObject::tr("Run script file"),
                                    QObject::tr("file"));
    cmd_parser.addOption(scriptOption);

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
        if (msg.exec() == QMessageBox::No) {
            std::exit(1);
        }
    }

#ifdef CUTTER_ENABLE_PYTHON
    // Init python
    if (cmd_parser.isSet(pythonHomeOption)) {
        Python()->setPythonHome(cmd_parser.value(pythonHomeOption));
    }
    Python()->initialize();
#endif

    Core()->initialize();
    Core()->setSettings();
    Config()->loadInitial();

    bool analLevelSpecified = false;
    int analLevel = 0;

    if (cmd_parser.isSet(analOption)) {
        analLevel = cmd_parser.value(analOption).toInt(&analLevelSpecified);

        if (!analLevelSpecified || analLevel < 0 || analLevel > 2) {
            printf("%s\n",
                   QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.").toLocal8Bit().constData());
            std::exit(1);
        }
    }

    Plugins()->loadPlugins();

    mainWindow = new MainWindow();
    installEventFilter(mainWindow);

    // set up context menu shortcut display fix
#if QT_VERSION_CHECK(5, 10, 0) < QT_VERSION
    setStyle(new CutterProxyStyle());
#endif // QT_VERSION_CHECK(5, 10, 0) < QT_VERSION

    if (args.empty()) {
        if (analLevelSpecified) {
            printf("%s\n",
                   QObject::tr("Filename must be specified to start analysis automatically.").toLocal8Bit().constData());
            std::exit(1);
        }

        // check if this is the first execution of Cutter in this computer
        // Note: the execution after the preferences benn reset, will be considered as first-execution
        if (Config()->isFirstExecution()) {
            mainWindow->displayWelcomeDialog();
        }
        mainWindow->displayNewFileDialog();
    } else { // filename specified as positional argument
        InitialOptions options;
        options.filename = args[0];
        if (analLevelSpecified) {
            switch (analLevel) {
            case 0:
            default:
                options.analCmd = {};
                break;
            case 1:
                options.analCmd = { "aaa" };
                break;
            case 2:
                options.analCmd = { "aaaa" };
                break;
            }
        }
        options.script = cmd_parser.value(scriptOption);
        mainWindow->openNewFile(options, analLevelSpecified);
    }

#ifdef CUTTER_APPVEYOR_R2DEC
    qputenv("R2DEC_HOME", "radare2\\lib\\plugins\\r2dec-js");
#endif
}

CutterApplication::~CutterApplication()
{
#ifdef CUTTER_ENABLE_PYTHON
    Plugins()->destroyPlugins();
#endif
    delete mainWindow;
#ifdef CUTTER_ENABLE_PYTHON
    Python()->shutdown();
#endif
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
                InitialOptions options;
                options.filename = fileName;
                mainWindow->openNewFile(options);
            }
        }
    }
    return QApplication::event(e);
}

bool CutterApplication::loadTranslations()
{
    const QString &language = Config()->getCurrLocale().bcp47Name();
    if (language == QStringLiteral("en") || language.startsWith(QStringLiteral("en-"))) {
        return true;
    }
    const auto &allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
        QLocale::AnyCountry);

    bool cutterTrLoaded = false;

    for (const QLocale &it : allLocales) {
        const QString &langPrefix = it.bcp47Name();
        if (langPrefix == language) {
            QApplication::setLayoutDirection(it.textDirection());
            QLocale::setDefault(it);

            QTranslator *trCutter = new QTranslator;
            QTranslator *trQtBase = new QTranslator;
            QTranslator *trQt = new QTranslator;

            const QStringList &cutterTrPaths = Config()->getTranslationsDirectories();

            for (const auto &trPath : cutterTrPaths) {
                if (trCutter && trCutter->load(it, QLatin1String("cutter"), QLatin1String("_"), trPath)) {
                    installTranslator(trCutter);
                    cutterTrLoaded = true;
                    trCutter = nullptr;
                }
                if (trQt && trQt->load(it, "qt", "_", trPath)) {
                    installTranslator(trQt);
                    trQt = nullptr;
                }

                if (trQtBase && trQtBase->load(it, "qtbase", "_", trPath)) {
                    installTranslator(trQtBase);
                    trQtBase = nullptr;
                }
            }
            
            if (trCutter) {
                delete trCutter;
            }
            if (trQt) {
                delete trQt;
            }
            if (trQtBase) {
                delete trQtBase;
            }
            return true;
        }
    }
    if (!cutterTrLoaded) {
        qWarning() << "Cannot load Cutter's translation for " << language;
    }
    return false;
}


void CutterProxyStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
#if QT_VERSION_CHECK(5, 10, 0) < QT_VERSION
    // HACK: This is the only way I've found to force Qt (5.10 and newer) to
    //       display shortcuts in context menus on all platforms. It's ugly,
    //       but it gets the job done.
    if (auto menu = qobject_cast<QMenu*>(widget)) {
        const auto &actions = menu->actions();
        for (auto action : actions) {
            action->setShortcutVisibleInContextMenu(true);
        }
    }
#endif // QT_VERSION_CHECK(5, 10, 0) < QT_VERSION
}

