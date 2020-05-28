#include "common/PythonManager.h"
#include "common/CrashHandler.h"
#include "CutterApplication.h"
#include "plugins/PluginManager.h"
#include "CutterConfig.h"
#include "common/Decompiler.h"

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
#include <QFontDatabase>
#ifdef Q_OS_WIN
#include <QtNetwork/QtNetwork>
#endif // Q_OS_WIN

#include <cstdlib>

#if CUTTER_R2GHIDRA_STATIC
#include <R2GhidraDecompiler.h>
#endif

CutterApplication::CutterApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    // Setup application information
    setApplicationVersion(CUTTER_VERSION_FULL);
    setWindowIcon(QIcon(":/img/cutter.svg"));
    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setLayoutDirection(Qt::LeftToRight);

    // WARN!!! Put initialization code below this line. Code above this line is mandatory to be run First

#ifdef Q_OS_WIN
    // Hack to force Cutter load internet connection related DLL's
    QSslSocket s;
    s.sslConfiguration();
#endif // Q_OS_WIN

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

    if (!parseCommandLineOptions()) {
        std::exit(1);
    }

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
    if (!clOptions.pythonHome.isEmpty()) {
        Python()->setPythonHome(clOptions.pythonHome);
    }
    Python()->initialize();
#endif

#ifdef Q_OS_WIN
    // Redefine r_sys_prefix() behaviour
    qputenv("R_ALT_SRC_DIR", "1");
#endif

    Core()->initialize(clOptions.enableR2Plugins);
    Core()->setSettings();
    Config()->loadInitial();
    Core()->loadCutterRC();

    Config()->setOutputRedirectionEnabled(clOptions.outputRedirectionEnabled);

    if (R2DecDecompiler::isAvailable()) {
        Core()->registerDecompiler(new R2DecDecompiler(Core()));
    }

#if CUTTER_R2GHIDRA_STATIC
    Core()->registerDecompiler(new R2GhidraDecompiler(Core()));
#endif

    Plugins()->loadPlugins(clOptions.enableCutterPlugins);

    for (auto &plugin : Plugins()->getPlugins()) {
        plugin->registerDecompilers();
    }

    mainWindow = new MainWindow();
    installEventFilter(mainWindow);

    // set up context menu shortcut display fix
#if QT_VERSION_CHECK(5, 10, 0) < QT_VERSION
    setStyle(new CutterProxyStyle());
#endif // QT_VERSION_CHECK(5, 10, 0) < QT_VERSION

    if (clOptions.args.empty()) {
        // check if this is the first execution of Cutter in this computer
        // Note: the execution after the preferences been reset, will be considered as first-execution
        if (Config()->isFirstExecution()) {
            mainWindow->displayWelcomeDialog();
        }
        mainWindow->displayNewFileDialog();
    } else { // filename specified as positional argument
        bool askOptions = clOptions.analLevel != AutomaticAnalysisLevel::Ask;
        mainWindow->openNewFile(clOptions.fileOpenOptions, askOptions);
    }

#ifdef CUTTER_APPVEYOR_R2DEC
    qputenv("R2DEC_HOME", "radare2\\lib\\plugins\\r2dec-js");
#endif

#ifdef APPIMAGE
    {
        auto appdir = QDir(QCoreApplication::applicationDirPath()); // appdir/bin
        appdir.cdUp(); // appdir

        auto sleighHome = appdir;
        sleighHome.cd("share/radare2/plugins/r2ghidra_sleigh"); // appdir/share/radare2/plugins/r2ghidra_sleigh
        Core()->setConfig("r2ghidra.sleighhome", sleighHome.absolutePath());

        auto r2decHome = appdir;
        r2decHome.cd("share/radare2/plugins/r2dec-js"); // appdir/share/radare2/plugins/r2dec-js
        qputenv("R2DEC_HOME", r2decHome.absolutePath().toLocal8Bit());
    }
#endif

#ifdef Q_OS_MACOS
    {
        auto r2prefix = QDir(QCoreApplication::applicationDirPath()); // Contents/MacOS
        r2prefix.cdUp(); // Contents
        r2prefix.cd("Resources/r2"); // Contents/Resources/r2

        auto sleighHome = r2prefix;
        sleighHome.cd("share/radare2/plugins/r2ghidra_sleigh"); // Contents/Resources/r2/share/radare2/plugins/r2ghidra_sleigh
        Core()->setConfig("r2ghidra.sleighhome", sleighHome.absolutePath());

        auto r2decHome = r2prefix;
        r2decHome.cd("share/radare2/plugins/r2dec-js"); // Contents/Resources/r2/share/radare2/plugins/r2dec-js
        qputenv("R2DEC_HOME", r2decHome.absolutePath().toLocal8Bit());
    }
#endif

#ifdef Q_OS_WIN
    {
        auto sleighHome = QDir(QCoreApplication::applicationDirPath());
        sleighHome.cd("radare2/lib/plugins/r2ghidra_sleigh");
        Core()->setConfig("r2ghidra.sleighhome", sleighHome.absolutePath());
    }
#endif
}

CutterApplication::~CutterApplication()
{
    Plugins()->destroyPlugins();
    delete mainWindow;
#ifdef CUTTER_ENABLE_PYTHON
    Python()->shutdown();
#endif
}

void CutterApplication::launchNewInstance(const QStringList &args)
{
    QProcess process(this);
    process.setEnvironment(QProcess::systemEnvironment());
    QStringList allArgs;
    if (!clOptions.enableCutterPlugins) {
        allArgs.push_back("--no-cutter-plugins");
    }
    if (!clOptions.enableR2Plugins) {
        allArgs.push_back("--no-r2-plugins");
    }
    allArgs.append(args);
    process.startDetached(qApp->applicationFilePath(), allArgs);
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
                launchNewInstance({fileName});
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

bool CutterApplication::parseCommandLineOptions()
{
    // Keep this function in sync with documentation

    QCommandLineParser cmd_parser;
    cmd_parser.setApplicationDescription(
        QObject::tr("A Qt and C++ GUI for radare2 reverse engineering framework"));
    cmd_parser.addHelpOption();
    cmd_parser.addVersionOption();
    cmd_parser.addPositionalArgument("filename", QObject::tr("Filename to open."));

    QCommandLineOption analOption({"A", "analysis"},
                                  QObject::tr("Automatically open file and optionally start analysis. "
                                              "Needs filename to be specified. May be a value between 0 and 2:"
                                              " 0 = no analysis, 1 = aaa, 2 = aaaa (experimental)"),
                                  QObject::tr("level"));
    cmd_parser.addOption(analOption);

    QCommandLineOption formatOption({"F", "format"},
                                    QObject::tr("Force using a specific file format (bin plugin)"),
                                    QObject::tr("name"));
    cmd_parser.addOption(formatOption);

    QCommandLineOption baddrOption({"B", "base"},
                                   QObject::tr("Load binary at a specific base address"),
                                   QObject::tr("base address"));
    cmd_parser.addOption(baddrOption);

    QCommandLineOption scriptOption("i",
                                    QObject::tr("Run script file"),
                                    QObject::tr("file"));
    cmd_parser.addOption(scriptOption);

    QCommandLineOption writeModeOption({"w", "writemode"},
                                       QObject::tr("Open file in write mode"));
    cmd_parser.addOption(writeModeOption);


    QCommandLineOption pythonHomeOption("pythonhome",
                                        QObject::tr("PYTHONHOME to use for embedded python interpreter"),
                                        "PYTHONHOME");
    cmd_parser.addOption(pythonHomeOption);

    QCommandLineOption disableRedirectOption("no-output-redirect",
                                             QObject::tr("Disable output redirection."
                                                         " Some of the output in console widget will not be visible."
                                                         " Use this option when debuging a crash or freeze and output "
                                                         " redirection is causing some messages to be lost."));
    cmd_parser.addOption(disableRedirectOption);

    QCommandLineOption disablePlugins("no-plugins",
                                      QObject::tr("Do not load plugins"));
    cmd_parser.addOption(disablePlugins);

    QCommandLineOption disableCutterPlugins("no-cutter-plugins",
                                            QObject::tr("Do not load Cutter plugins"));
    cmd_parser.addOption(disableCutterPlugins);

    QCommandLineOption disableR2Plugins("no-r2-plugins",
                                        QObject::tr("Do not load radare2 plugins"));
    cmd_parser.addOption(disableR2Plugins);

    cmd_parser.process(*this);

    CutterCommandLineOptions opts;
    opts.args = cmd_parser.positionalArguments();

    if (cmd_parser.isSet(analOption)) {
        bool analLevelSpecified = false;
        int analLevel = cmd_parser.value(analOption).toInt(&analLevelSpecified);

        if (!analLevelSpecified || analLevel < 0 || analLevel > 2) {
            fprintf(stderr, "%s\n",
                    QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.").toLocal8Bit().constData());
            return false;
        }
        switch (analLevel) {
        case 0:
            opts.analLevel = AutomaticAnalysisLevel::None;
            break;
        case 1:
            opts.analLevel = AutomaticAnalysisLevel::AAA;
            break;
        case 2:
            opts.analLevel = AutomaticAnalysisLevel::AAAA;
            break;
        }
    }

    if (opts.args.empty() && opts.analLevel != AutomaticAnalysisLevel::Ask) {
        fprintf(stderr, "%s\n",
                QObject::tr("Filename must be specified to start analysis automatically.").toLocal8Bit().constData());
        return false;
    }

    InitialOptions options;
    if (!opts.args.isEmpty()) {
        opts.fileOpenOptions.filename = opts.args[0];
        opts.fileOpenOptions.forceBinPlugin = cmd_parser.value(formatOption);
        if (cmd_parser.isSet(baddrOption)) {
            bool ok;
            RVA baddr = cmd_parser.value(baddrOption).toULongLong(&ok, 0);
            if (ok) {
                options.binLoadAddr = baddr;
            }
        }
        switch (opts.analLevel) {
        case AutomaticAnalysisLevel::Ask:
            break;
        case AutomaticAnalysisLevel::None:
            opts.fileOpenOptions.analCmd = {};
            break;
        case AutomaticAnalysisLevel::AAA:
            opts.fileOpenOptions.analCmd = { {"aaa", "Auto analysis"} };
            break;
        case AutomaticAnalysisLevel::AAAA:
            opts.fileOpenOptions.analCmd = { {"aaaa", "Auto analysis (experimental)"} };
            break;
        }
        opts.fileOpenOptions.script = cmd_parser.value(scriptOption);

        opts.fileOpenOptions.writeEnabled = cmd_parser.isSet(writeModeOption);
    }

    if (cmd_parser.isSet(pythonHomeOption)) {
        opts.pythonHome = cmd_parser.value(pythonHomeOption);
    }

    opts.outputRedirectionEnabled = !cmd_parser.isSet(disableRedirectOption);
    if (cmd_parser.isSet(disablePlugins)) {
        opts.enableCutterPlugins = false;
        opts.enableR2Plugins = false;
    }

    if (cmd_parser.isSet(disableCutterPlugins)) {
        opts.enableCutterPlugins = false;
    }

    if (cmd_parser.isSet(disableR2Plugins)) {
        opts.enableR2Plugins = false;
    }

    this->clOptions = opts;
    return true;
}


void CutterProxyStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
#if QT_VERSION_CHECK(5, 10, 0) < QT_VERSION
    // HACK: This is the only way I've found to force Qt (5.10 and newer) to
    //       display shortcuts in context menus on all platforms. It's ugly,
    //       but it gets the job done.
    if (auto menu = qobject_cast<QMenu *>(widget)) {
        const auto &actions = menu->actions();
        for (auto action : actions) {
            action->setShortcutVisibleInContextMenu(true);
        }
    }
#endif // QT_VERSION_CHECK(5, 10, 0) < QT_VERSION
}
