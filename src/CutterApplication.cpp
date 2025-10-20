#include "common/PythonManager.h"
#include "CutterApplication.h"
#include "plugins/PluginManager.h"
#include "CutterConfig.h"
#include "common/Decompiler.h"
#include "common/ResourcePaths.h"

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
#    include <QtNetwork/QtNetwork>
#endif // Q_OS_WIN

#include <cstdlib>

#if CUTTER_RZGHIDRA_STATIC
#    include <RzGhidraDecompiler.h>
#endif

// Rizin before 301e5af2170d9f3ed1edd658b0f9633f31fc4126
// has RZ_GITTAP defined and uses it in rz_core_version().
// After that, RZ_GITTAP is not defined anymore and RZ_VERSION is used.
#ifdef RZ_GITTAP
#    define CUTTER_COMPILE_TIME_RZ_VERSION "" RZ_GITTAP
#else
#    define CUTTER_COMPILE_TIME_RZ_VERSION "" RZ_VERSION
#endif

CutterApplication::CutterApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    // Setup application information
    setApplicationVersion(CUTTER_VERSION_FULL);
#ifdef Q_OS_MACOS
    setWindowIcon(QIcon(":/img/cutter_macos_simple.svg"));
#else
    setWindowIcon(QIcon(":/img/cutter.svg"));
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setAttribute(Qt::AA_UseHighDpiPixmaps); // always enabled on Qt >= 6.0.0
#endif
    setLayoutDirection(Qt::LeftToRight);
    // WARN!!! Put initialization code below this line. Code above this line is mandatory to be run
    // First

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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif

    if (!parseCommandLineOptions()) {
        std::exit(1);
    }

    // Check rizin version
    QString rzversion = rz_core_version();
    QString localVersion = CUTTER_COMPILE_TIME_RZ_VERSION;
    qDebug() << rzversion << localVersion;
    if (rzversion != localVersion) {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setWindowTitle(QObject::tr("Version mismatch!"));
        msg.setText(QString(QObject::tr("The version used to compile Cutter (%1) does not match "
                                        "the binary version of rizin (%2). This could result in "
                                        "unexpected behaviour. Are you sure you want to continue?"))
                            .arg(localVersion, rzversion));
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

    Core()->initialize(clOptions.enableRizinPlugins);
    Core()->setSettings();
    Config()->loadInitial();
    Core()->loadCutterRC();

    Config()->setOutputRedirectionEnabled(clOptions.outputRedirectionEnabled);

#if CUTTER_RZGHIDRA_STATIC
    Core()->registerDecompiler(new RzGhidraDecompiler(Core()));
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

    if (clOptions.args.empty() && clOptions.fileOpenOptions.projectFile.isEmpty()) {
        // check if this is the first execution of Cutter in this computer
        // Note: the execution after the preferences been reset, will be considered as
        // first-execution
        if (Config()->isFirstExecution()) {
            mainWindow->displayWelcomeDialog();
        }
        mainWindow->displayNewFileDialog();
    } else { // filename specified as positional argument
        bool askOptions = (clOptions.analysisLevel != AutomaticAnalysisLevel::Ask)
                || !clOptions.fileOpenOptions.projectFile.isEmpty();
        mainWindow->openNewFile(clOptions.fileOpenOptions, askOptions);
    }

#ifdef APPIMAGE
    {
        auto appdir = QDir(QCoreApplication::applicationDirPath()); // appdir/bin
        appdir.cdUp(); // appdir

        auto sleighHome = appdir;
        // appdir/lib/rizin/plugins/rz_ghidra_sleigh/
        sleighHome.cd("lib/rizin/plugins/rz_ghidra_sleigh/");
        Core()->setConfig("ghidra.sleighhome", sleighHome.absolutePath());
    }
#endif

#if defined(Q_OS_MACOS) && defined(CUTTER_ENABLE_PACKAGING)
    {
        auto rzprefix = QDir(QCoreApplication::applicationDirPath()); // Contents/MacOS
        rzprefix.cdUp(); // Contents
        rzprefix.cd("Resources"); // Contents/Resources/

        auto sleighHome = rzprefix;
        // Contents/Resources/lib/rizin/plugins/rz_ghidra_sleigh
        sleighHome.cd("lib/rizin/plugins/rz_ghidra_sleigh");
        Core()->setConfig("ghidra.sleighhome", sleighHome.absolutePath());
    }
#endif

#if defined(Q_OS_WIN) && defined(CUTTER_ENABLE_PACKAGING)
    {
        auto sleighHome = QDir(QCoreApplication::applicationDirPath());
        sleighHome.cd("lib/rizin/plugins/rz_ghidra_sleigh");
        Core()->setConfig("ghidra.sleighhome", sleighHome.absolutePath());
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
    if (!clOptions.enableRizinPlugins) {
        allArgs.push_back("--no-rizin-plugins");
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
                launchNewInstance({ fileName });
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
    auto locale = Config()->getCurrLocale();

    bool cutterTrLoaded = false;

    auto availableTranslations = Config()->getAvailableTranslations();
    if (std::find_if(availableTranslations.begin(), availableTranslations.end(),
                     [&](const Configuration::LangInfo &item) { return locale == item.locale; })
        != availableTranslations.end()) {
        QApplication::setLayoutDirection(locale.textDirection());
        QLocale::setDefault(locale);

        QTranslator *trCutter = new QTranslator;
        QTranslator *trQtBase = new QTranslator;
        QTranslator *trQt = new QTranslator;

        const QStringList &cutterTrPaths = Cutter::getTranslationsDirectories();

        for (const auto &trPath : cutterTrPaths) {
            if (trCutter
                && trCutter->load(locale, QLatin1String("cutter"), QLatin1String("_"), trPath)) {
                installTranslator(trCutter);
                cutterTrLoaded = true;
                trCutter = nullptr;
            }
            if (trQt && trQt->load(locale, "qt", "_", trPath)) {
                installTranslator(trQt);
                trQt = nullptr;
            }

            if (trQtBase && trQtBase->load(locale, "qtbase", "_", trPath)) {
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
    if (locale.language() == QLocale::English) {
        return true;
    }
    if (!cutterTrLoaded) {
        qWarning() << "Cannot load Cutter's translation for " << locale.name();
    }
    return false;
}

QStringList CutterApplication::getArgs() const
{
    auto &options = clOptions.fileOpenOptions;

    QStringList args;
    switch (clOptions.analysisLevel) {
    case AutomaticAnalysisLevel::None:
        args.push_back("-A");
        args.push_back("0");
        break;
    case AutomaticAnalysisLevel::AAA:
        args.push_back("-A");
        args.push_back("1");
        break;
    case AutomaticAnalysisLevel::AAAA:
        args.push_back("-A");
        args.push_back("2");
        break;
    default:
        break;
    }

    if (!options.useVA) {
        args.push_back("-P");
    }
    if (options.writeEnabled) {
        args.push_back("-w");
    }
    if (!options.script.isEmpty()) {
        args.push_back("-i");
        args.push_back(options.script);
    }
    if (!options.projectFile.isEmpty()) {
        args.push_back("-p");
        args.push_back(options.projectFile);
    }
    if (!options.arch.isEmpty()) {
        args.push_back("-a");
        args.push_back(options.arch);
    }
    if (options.bits > 0) {
        args.push_back("-b");
        args.push_back(QString::asprintf("%d", options.bits));
    }
    if (!options.cpu.isEmpty()) {
        args.push_back("-c");
        args.push_back(options.cpu);
    }
    if (!options.os.isEmpty()) {
        args.push_back("-o");
        args.push_back(options.os);
    }

    switch (options.endian) {
    case InitialOptions::Endianness::Little:
        args.push_back("-e");
        args.push_back("little");
        break;
    case InitialOptions::Endianness::Big:
        args.push_back("-e");
        args.push_back("big");
        break;
    default:
        break;
    }
    if (!options.forceBinPlugin.isEmpty()) {
        args.push_back("-F");
        args.push_back(options.forceBinPlugin);
    }
    if (options.binLoadAddr != RVA_INVALID) {
        args.push_back("-B");
        args.push_back(RzAddressString(options.binLoadAddr));
    }
    if (options.mapAddr != RVA_INVALID) {
        args.push_back("-m");
        args.push_back(RzAddressString(options.mapAddr));
    }
    if (!options.filename.isEmpty()) {
        args.push_back(options.filename);
    }
    return args;
}

bool CutterApplication::parseCommandLineOptions()
{
    // Keep this function in sync with documentation

    QCommandLineParser cmd_parser;
    cmd_parser.setApplicationDescription(
            QObject::tr("A Qt and C++ GUI for rizin reverse engineering framework"));
    cmd_parser.addHelpOption();
    cmd_parser.addVersionOption();
    cmd_parser.addPositionalArgument("filename", QObject::tr("Filename to open."));

    QCommandLineOption analOption(
            { "A", "analysis" },
            QObject::tr("Automatically open file and optionally start analysis. "
                        "Needs filename to be specified. May be a value between 0 and 2:"
                        " 0 = no analysis, 1 = aaa, 2 = aaaa (experimental)"),
            QObject::tr("level"));
    cmd_parser.addOption(analOption);

    QCommandLineOption archOption({ "a", "arch" }, QObject::tr("Sets a specific architecture name"),
                                  QObject::tr("arch"));
    cmd_parser.addOption(archOption);

    QCommandLineOption bitsOption({ "b", "bits" }, QObject::tr("Sets a specific architecture bits"),
                                  QObject::tr("bits"));
    cmd_parser.addOption(bitsOption);

    QCommandLineOption cpuOption({ "c", "cpu" }, QObject::tr("Sets a specific CPU"),
                                 QObject::tr("cpu"));
    cmd_parser.addOption(cpuOption);

    QCommandLineOption osOption({ "o", "os" }, QObject::tr("Sets a specific operating system"),
                                QObject::tr("os"));
    cmd_parser.addOption(osOption);

    QCommandLineOption endianOption({ "e", "endian" },
                                    QObject::tr("Sets the endianness (big or little)"),
                                    QObject::tr("big|little"));
    cmd_parser.addOption(endianOption);

    QCommandLineOption formatOption({ "F", "format" },
                                    QObject::tr("Force using a specific file format (bin plugin)"),
                                    QObject::tr("name"));
    cmd_parser.addOption(formatOption);

    QCommandLineOption baddrOption({ "B", "base" },
                                   QObject::tr("Load binary at a specific base address"),
                                   QObject::tr("base address"));
    cmd_parser.addOption(baddrOption);

    QCommandLineOption maddrOption({ "m", "map" },
                                   QObject::tr("Map the binary at a specific address"),
                                   QObject::tr("map address"));
    cmd_parser.addOption(maddrOption);

    QCommandLineOption scriptOption("i", QObject::tr("Run script file"), QObject::tr("file"));
    cmd_parser.addOption(scriptOption);

    QCommandLineOption projectOption({ "p", "project" }, QObject::tr("Load project file"),
                                     QObject::tr("project file"));
    cmd_parser.addOption(projectOption);

    QCommandLineOption writeModeOption({ "w", "writemode" },
                                       QObject::tr("Open file in write mode"));
    cmd_parser.addOption(writeModeOption);

    QCommandLineOption phyModeOption({ "P", "phymode" },
                                     QObject::tr("Disables virtual addressing"));
    cmd_parser.addOption(phyModeOption);

    QCommandLineOption pythonHomeOption(
            "pythonhome", QObject::tr("PYTHONHOME to use for embedded python interpreter"),
            "PYTHONHOME");
    cmd_parser.addOption(pythonHomeOption);

    QCommandLineOption disableRedirectOption(
            "no-output-redirect",
            QObject::tr("Disable output redirection."
                        " Some of the output in console widget will not be visible."
                        " Use this option when debuging a crash or freeze and output "
                        " redirection is causing some messages to be lost."));
    cmd_parser.addOption(disableRedirectOption);

    QCommandLineOption disablePlugins("no-plugins", QObject::tr("Do not load plugins"));
    cmd_parser.addOption(disablePlugins);

    QCommandLineOption disableCutterPlugins("no-cutter-plugins",
                                            QObject::tr("Do not load Cutter plugins"));
    cmd_parser.addOption(disableCutterPlugins);

    QCommandLineOption disableRizinPlugins("no-rizin-plugins",
                                           QObject::tr("Do not load rizin plugins"));
    cmd_parser.addOption(disableRizinPlugins);

    cmd_parser.process(*this);

    CutterCommandLineOptions opts;
    opts.args = cmd_parser.positionalArguments();

    if (cmd_parser.isSet(analOption)) {
        bool analysisLevelSpecified = false;
        int analysisLevel = cmd_parser.value(analOption).toInt(&analysisLevelSpecified);

        if (!analysisLevelSpecified || analysisLevel < 0 || analysisLevel > 2) {
            fprintf(stderr, "%s\n",
                    QObject::tr("Invalid Analysis Level. May be a value between 0 and 2.")
                            .toLocal8Bit()
                            .constData());
            return false;
        }
        switch (analysisLevel) {
        case 0:
            opts.analysisLevel = AutomaticAnalysisLevel::None;
            break;
        case 1:
            opts.analysisLevel = AutomaticAnalysisLevel::AAA;
            break;
        case 2:
            opts.analysisLevel = AutomaticAnalysisLevel::AAAA;
            break;
        }
    }

    if (opts.args.empty() && opts.analysisLevel != AutomaticAnalysisLevel::Ask) {
        fprintf(stderr, "%s\n",
                QObject::tr("Filename must be specified to start analysis automatically.")
                        .toLocal8Bit()
                        .constData());
        return false;
    }

    if (!opts.args.isEmpty()) {
        opts.fileOpenOptions.filename = opts.args[0];
        opts.fileOpenOptions.forceBinPlugin = cmd_parser.value(formatOption);
        if (cmd_parser.isSet(baddrOption)) {
            bool ok = false;
            RVA baddr = cmd_parser.value(baddrOption).toULongLong(&ok, 0);
            if (ok) {
                opts.fileOpenOptions.binLoadAddr = baddr;
            }
        }
        if (cmd_parser.isSet(maddrOption)) {
            bool ok = false;
            RVA maddr = cmd_parser.value(maddrOption).toULongLong(&ok, 0);
            if (ok) {
                opts.fileOpenOptions.mapAddr = maddr;
            }
        }
        switch (opts.analysisLevel) {
        case AutomaticAnalysisLevel::Ask:
            break;
        case AutomaticAnalysisLevel::None:
            opts.fileOpenOptions.analysisCmd = {};
            break;
        case AutomaticAnalysisLevel::AAA:
            opts.fileOpenOptions.analysisCmd = {
                { "aaa", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Auto analysis") }
            };
            break;
        case AutomaticAnalysisLevel::AAAA:
            opts.fileOpenOptions.analysisCmd = {
                { "aaaa",
                  QT_TRANSLATE_NOOP("InitialOptionsDialog", "Auto analysis (experimental)") }
            };
            break;
        }
        opts.fileOpenOptions.script = cmd_parser.value(scriptOption);
        opts.fileOpenOptions.arch = cmd_parser.value(archOption);
        opts.fileOpenOptions.cpu = cmd_parser.value(cpuOption);
        opts.fileOpenOptions.os = cmd_parser.value(osOption);
        if (cmd_parser.isSet(bitsOption)) {
            bool ok = false;
            int bits = cmd_parser.value(bitsOption).toInt(&ok, 10);
            if (ok && bits > 0) {
                opts.fileOpenOptions.bits = bits;
            }
        }
        if (cmd_parser.isSet(endianOption)) {
            QString endian = cmd_parser.value(endianOption).toLower();
            opts.fileOpenOptions.endian = InitialOptions::Endianness::Auto;
            if (endian == "little") {
                opts.fileOpenOptions.endian = InitialOptions::Endianness::Little;
            } else if (endian == "big") {
                opts.fileOpenOptions.endian = InitialOptions::Endianness::Big;
            } else {
                fprintf(stderr, "%s\n",
                        QObject::tr("Invalid Endianness. You can only set it to `big` or `little`.")
                                .toLocal8Bit()
                                .constData());
                return false;
            }
        } else {
            opts.fileOpenOptions.endian = InitialOptions::Endianness::Auto;
        }

        opts.fileOpenOptions.writeEnabled = cmd_parser.isSet(writeModeOption);
        opts.fileOpenOptions.useVA = !cmd_parser.isSet(phyModeOption);
    }

    opts.fileOpenOptions.projectFile = cmd_parser.value(projectOption);

    if (cmd_parser.isSet(pythonHomeOption)) {
        opts.pythonHome = cmd_parser.value(pythonHomeOption);
    }

    opts.outputRedirectionEnabled = !cmd_parser.isSet(disableRedirectOption);
    if (cmd_parser.isSet(disablePlugins)) {
        opts.enableCutterPlugins = false;
        opts.enableRizinPlugins = false;
    }

    if (cmd_parser.isSet(disableCutterPlugins)) {
        opts.enableCutterPlugins = false;
    }

    if (cmd_parser.isSet(disableRizinPlugins)) {
        opts.enableRizinPlugins = false;
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
