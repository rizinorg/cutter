#include "Configuration.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFontDatabase>
#include <QFile>
#include <QApplication>
#include <QLibraryInfo>

#include "common/ColorSchemeFileSaver.h"

const QList<CutterQtTheme> kCutterQtThemesList = {
    { "Native", static_cast<ColorFlags>(LightFlag | DarkFlag) },
    { "Dark",    DarkFlag }
};

Configuration *Configuration::mPtr = nullptr;

/*!
 * \brief All asm.* options saved as settings. Values are the default values.
 */
static const QHash<QString, QVariant> asmOptions = {
    { "asm.esil",           false },
    { "asm.pseudo",         false },
    { "asm.offset",         true },
    { "asm.xrefs",          false },
    { "asm.indent",         false },
    { "asm.describe",       false },
    { "asm.stackptr",       false },
    { "asm.slow",           true },
    { "asm.lines",          true },
    { "asm.lines.fcn",      true },
    { "asm.flags.offset",   false },
    { "asm.emu",            false },
    { "asm.cmt.right",      true },
    { "asm.cmt.col",        35 },
    { "asm.var.summary",    false },
    { "asm.bytes",          false },
    { "asm.size",           false },
    { "asm.bytespace",      false },
    { "asm.lbytes",         true },
    { "asm.nbytes",         10 },
    { "asm.syntax",         "intel" },
    { "asm.ucase",          false },
    { "asm.bbline",         false },
    { "asm.capitalize",     false },
    { "asm.var.sub",        true },
    { "asm.var.subonly",    true },
    { "asm.tabs",           5 },
    { "asm.tabs.off",       5 },
    { "asm.marks",          false },
    { "esil.breakoninvalid",   true },
    { "graph.offset",       false}
};


Configuration::Configuration() : QObject()
{
    mPtr = this;
    if (!s.isWritable()) {
        QMessageBox::critical(nullptr,
                              tr("Critical!"),
                              tr("!!! Settings are not writable! Make sure you have a write access to \"%1\"")
                              .arg(s.fileName())
                             );
    }
}

Configuration *Configuration::instance()
{
    if (!mPtr)
        mPtr = new Configuration();
    return mPtr;
}

void Configuration::loadInitial()
{
    setTheme(getTheme());
    setColorTheme(getColorTheme());
    applySavedAsmOptions();
}

QString Configuration::getDirProjects()
{
    auto projectsDir = s.value("dir.projects").toString();
    if (projectsDir.isEmpty()) {
        projectsDir = Core()->getConfig("dir.projects");
        setDirProjects(projectsDir);
    }

    return QDir::toNativeSeparators(projectsDir);
}

void Configuration::setDirProjects(const QString &dir)
{
    s.setValue("dir.projects", QDir::toNativeSeparators(dir));
}

QString Configuration::getRecentFolder()
{
    QString recentFolder = s.value("dir.recentFolder", QDir::homePath()).toString();

    return QDir::toNativeSeparators(recentFolder);
}

void Configuration::setRecentFolder(const QString &dir)
{
    s.setValue("dir.recentFolder", QDir::toNativeSeparators(dir));
}

/**
 * @brief Configuration::setFilesTabLastClicked
 * Set the new file dialog last clicked tab
 * @param lastClicked
 */
void Configuration::setNewFileLastClicked(int lastClicked)
{
    s.setValue("newFileLastClicked", lastClicked);
}

int Configuration::getNewFileLastClicked()
{
    return s.value("newFileLastClicked").toInt();
}

void Configuration::resetAll()
{
    Core()->cmd("e-");
    Core()->setSettings();
    // Delete the file so no extra configuration is in it.
    QFile settingsFile(s.fileName());
    settingsFile.remove();
    s.clear();

    loadInitial();
    emit fontsUpdated();
}

/*!
 * \brief get the current Locale set in Cutter's user configuration
 * \return a QLocale object describes user's current locale
 */
QLocale Configuration::getCurrLocale() const
{
    return s.value("locale", QLocale().system()).toLocale();
}

/*!
 * \brief sets Cutter's locale
 * \param l - a QLocale object describes the locate to configure
 */
void Configuration::setLocale(const QLocale &l)
{
    s.setValue("locale", l);
}

/*!
 * \brief set Cutter's interface language by a given locale name
 * \param language - a string represents the name of a locale language
 * \return true on success
 */
bool Configuration::setLocaleByName(const QString &language)
{
    const auto &allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                                      QLocale::AnyCountry);

    for (auto &it : allLocales) {
        if (QString::compare(it.nativeLanguageName(), language, Qt::CaseInsensitive) == 0) {
            setLocale(it);
            return true;
        }
    }
    return false;
}

bool Configuration::windowColorIsDark()
{
    ColorFlags currentThemeColorFlags = getCurrentTheme()->flag;
    if (currentThemeColorFlags == ColorFlags::LightFlag) {
        return false;
    } else if (currentThemeColorFlags == ColorFlags::DarkFlag) {
        return true;
    }

    const QPalette &palette = qApp->palette();
    auto windowColor = palette.color(QPalette::Window).toRgb();
    return (windowColor.red() + windowColor.green() + windowColor.blue()) < 382;
}

void Configuration::loadBaseThemeNative()
{
    /* Load Qt Theme */
    qApp->setStyleSheet("");

    /* Colors */
    // GUI
    setColor("gui.cflow",   QColor(0, 0, 0));
    setColor("gui.dataoffset", QColor(0, 0, 0));
    // Custom
    setColor("gui.imports", QColor(50, 140, 255));
    setColor("gui.main", QColor(0, 128, 0));
    setColor("gui.navbar.err", QColor(255, 0, 0));
    setColor("gui.navbar.seek", QColor(233, 86, 86));
    setColor("gui.navbar.pc", QColor(66, 238, 244));
    setColor("gui.navbar.code", QColor(104, 229, 69));
    setColor("gui.navbar.str", QColor(69, 104, 229));
    setColor("gui.navbar.sym", QColor(229, 150, 69));
    setColor("gui.navbar.empty", QColor(100, 100, 100));
    setColor("gui.breakpoint_background", QColor(233, 143, 143));
    setColor("gui.item_invalid", QColor(155, 155, 155));
    setColor("gui.item_unsafe", QColor(255, 129, 123));
    setColor("gui.overview.node",  QColor(200, 200, 200));
}

void Configuration::loadNativeTheme()
{
    loadBaseThemeNative();

    if (windowColorIsDark()) {
        setColor("gui.border",  QColor(0, 0, 0));
        setColor("gui.background", QColor(30, 30, 30));
        setColor("gui.alt_background", QColor(42, 42, 42));
        setColor("gui.disass_selected", QColor(35, 35, 35));
        setColor("highlight",   QColor(255, 255, 255, 15));
        setColor("highlightWord", QColor(20, 20, 20, 255));
        setColor("highlightPC", QColor(87, 26, 7));
    } else {
        setColor("gui.border",  QColor(0, 0, 0));
        setColor("gui.background", QColor(255, 255, 255));
        setColor("gui.alt_background", QColor(245, 250, 255));
        setColor("gui.disass_selected", QColor(255, 255, 255));
        setColor("highlight",   QColor(210, 210, 255, 150));
        setColor("highlightWord", QColor(179, 119, 214, 60));
        setColor("highlightPC", QColor(214, 255, 210));
    }
}

void Configuration::loadBaseThemeDark()
{
    /* Load Qt Theme */
    QFile f(":qdarkstyle/style.qss");
    if (!f.exists()) {
        qWarning() << "Can't find dark theme stylesheet.";
    } else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        QString stylesheet = ts.readAll();
#ifdef Q_OS_MACX
        // see https://github.com/ColinDuquesnoy/QDarkStyleSheet/issues/22#issuecomment-96179529
        stylesheet += "QDockWidget::title"
                      "{"
                      "    background-color: #31363b;"
                      "    text-align: center;"
                      "    height: 12px;"
                      "}";
#endif
        qApp->setStyleSheet(stylesheet);
    }

    /* Colors */
    // GUI
    setColor("gui.cflow",   QColor(255, 255, 255));
    setColor("gui.dataoffset", QColor(255, 255, 255));
    // Custom
    setColor("gui.imports", QColor(50, 140, 255));
    setColor("gui.item_invalid", QColor(155, 155, 155));
    setColor("gui.item_unsafe", QColor(255, 129, 123));
    setColor("gui.main", QColor(0, 128, 0));

    // GUI: navbar
    setColor("gui.navbar.err", QColor(233, 86, 86));
    setColor("gui.navbar.seek", QColor(233, 86, 86));
    setColor("gui.navbar.pc", QColor(66, 238, 244));
    setColor("gui.navbar.code", QColor(130, 200, 111));
    setColor("angui.navbar.str", QColor(111, 134, 216));
    setColor("gui.navbar.sym", QColor(221, 163, 104));
    setColor("gui.navbar.empty", QColor(100, 100, 100));

    // RIP line selection in debug
    setColor("highlightPC", QColor(87, 26, 7));
    setColor("gui.breakpoint_background", QColor(140, 76, 76));

    setColor("gui.overview.node",  QColor(100, 100, 100));
}

void Configuration::loadDarkTheme()
{
    loadBaseThemeDark();
    setColor("gui.border",  QColor(100, 100, 100));
    // Windows background
    setColor("gui.background", QColor(37, 40, 43));
    // Disassembly nodes background
    setColor("gui.alt_background", QColor(28, 31, 36));
    // Disassembly nodes background when selected
    setColor("gui.disass_selected", QColor(31, 34, 40));
    // Disassembly line selected
    setColor("highlight", QColor(21, 29, 29, 150));
    setColor("highlightWord", QColor(52, 58, 71, 255));
}

const QFont Configuration::getFont() const
{
    QFont font = s.value("font", QFont("Inconsolata", 11)).value<QFont>();
    return font;
}

void Configuration::setFont(const QFont &font)
{
    s.setValue("font", font);
    emit fontsUpdated();
}

QString Configuration::getLastThemeOf(const CutterQtTheme &currQtTheme) const
{
    return s.value("lastThemeOf." + currQtTheme.name,
                   Config()->getColorTheme()).toString();
}

void Configuration::setTheme(int theme)
{
    if (theme >= kCutterQtThemesList.size() ||
            theme < 0) {
        theme = 0;
    }
    s.setValue("ColorPalette", theme);
    QString themeName = kCutterQtThemesList[theme].name;

    if (themeName == "Native") {
        loadNativeTheme();
    } else if (themeName == "Dark") {
        loadDarkTheme();
    } else {
        loadNativeTheme();
    }

    emit colorsUpdated();
}

const CutterQtTheme *Configuration::getCurrentTheme()
{
    int i = getTheme();
    if (i < 0 || i >= kCutterQtThemesList.size()) {
        i = 0;
        setTheme(i);
    }
    return &kCutterQtThemesList[i];
}

QString Configuration::getLogoFile()
{
    return windowColorIsDark()
           ? QString(":/img/cutter_white_plain.svg")
           : QString(":/img/cutter_plain.svg");
}

/*!
 * \brief Configuration::setColor sets the local Cutter configuration color
 * \param name Color Name
 * \param color The color you want to set
 */
void Configuration::setColor(const QString &name, const QColor &color)
{
    s.setValue("colors." + name, color);
}

void Configuration::setLastThemeOf(const CutterQtTheme &currQtTheme, const QString &theme)
{
    s.setValue("lastThemeOf." + currQtTheme.name, theme);
}

const QColor Configuration::getColor(const QString &name) const
{
    if (s.contains("colors." + name)) {
        return s.value("colors." + name).value<QColor>();
    } else {
        return s.value("colors.other").value<QColor>();
    }
}

void Configuration::setColorTheme(const QString &theme)
{
    if (theme == "default") {
        Core()->cmd("ecd");
        s.setValue("theme", "default");
    } else {
        Core()->cmd(QStringLiteral("eco %1").arg(theme));
        s.setValue("theme", theme);
    }
    // Duplicate interesting colors into our Cutter Settings
    // Dirty fix for arrow colors, TODO refactor getColor, setColor, etc.
    QJsonDocument colors = Core()->cmdj("ecj");
    QJsonObject colorsObject = colors.object();

    for (auto it = colorsObject.constBegin(); it != colorsObject.constEnd(); it++) {
        QJsonArray rgb = it.value().toArray();
        if (rgb.size() != 3) {
            continue;
        }
        s.setValue("colors." + it.key(), QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()));
    }

    QMap<QString, QColor> cutterSpecific = ColorSchemeFileWorker().getCutterSpecific();
    for (auto &it : cutterSpecific.keys())
        setColor(it, cutterSpecific[it]);

    if (!ColorSchemeFileWorker().isCustomScheme(theme)) {
        setTheme(getTheme());
    }

    emit colorsUpdated();
}

void Configuration::resetToDefaultAsmOptions()
{
    for (auto it = asmOptions.cbegin(); it != asmOptions.cend(); it++) {
        setConfig(it.key(), it.value());
    }
}

void Configuration::applySavedAsmOptions()
{
    for (auto it = asmOptions.cbegin(); it != asmOptions.cend(); it++) {
        Core()->setConfig(it.key(), s.value(it.key(), it.value()));
    }
}

QVariant Configuration::getConfigVar(const QString &key)
{
    QHash<QString, QVariant>::const_iterator it = asmOptions.find(key);
    if (it != asmOptions.end()) {
        switch (it.value().type()) {
        case QVariant::Type::Bool:
            return Core()->getConfigb(key);
        case QVariant::Type::Int:
            return Core()->getConfigi(key);
        default:
            return Core()->getConfig(key);
        }
    }
    return QVariant();
}


bool Configuration::getConfigBool(const QString &key)
{
    return getConfigVar(key).toBool();
}

int Configuration::getConfigInt(const QString &key)
{
    return getConfigVar(key).toInt();
}

QString Configuration::getConfigString(const QString &key)
{
    return getConfigVar(key).toString();
}

/**
 * @brief Configuration::setConfig
 * Set radare2 configuration value (e.g. "asm.lines")
 * @param key
 * @param value
 */
void Configuration::setConfig(const QString &key, const QVariant &value)
{
    if (asmOptions.contains(key)) {
        s.setValue(key, value);
    }

    Core()->setConfig(key, value);
}

/*!
 * \brief this function will gather and return available translation for Cutter
 * \return a list of all available translations
 */
QStringList Configuration::getAvailableTranslations()
{
    const auto &trDirs = getTranslationsDirectories();

    QSet<QString> fileNamesSet;
    for (const auto &trDir : trDirs) {
        QDir dir(trDir);
        if (!dir.exists()) {
            continue;
        }
        const QStringList &currTrFileNames = dir.entryList(QStringList("cutter_*.qm"), QDir::Files,
                                                           QDir::Name);
        for (const auto &trFile : currTrFileNames) {
            fileNamesSet << trFile;
        }
    }

    QStringList fileNames = fileNamesSet.toList();
    qSort(fileNames);
    QStringList languages;
    QString currLanguageName;
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                               QLocale::AnyCountry);

    for (auto i : fileNames) {
        QString localeName = i.mid(sizeof("cutter_") - 1, 2);
        for (auto j : allLocales) {
            if (j.name().startsWith(localeName)) {
                currLanguageName = j.nativeLanguageName();
                currLanguageName = currLanguageName.at(0).toUpper() +
                                   currLanguageName.right(currLanguageName.length() - 1);
                languages << currLanguageName;
                break;
            }
        }
    }
    return languages << QLatin1String("English");
}

/*!
 * \brief check if this is the first time Cutter's is executed on this computer
 * \return true if this is first execution; otherwise returns false.
 */
bool Configuration::isFirstExecution()
{
    // check if a variable named firstExecution existed in the configuration
    if (s.contains("firstExecution")) {
        return false;
    } else {
        s.setValue("firstExecution", false);
        return true;
    }
}

QStringList Configuration::getTranslationsDirectories() const
{
    static const QString cutterTranslationPath = QCoreApplication::applicationDirPath() +
                                                 QDir::separator()
                                                 + QLatin1String("translations");

    return {
        cutterTranslationPath,
        QLibraryInfo::location(QLibraryInfo::TranslationsPath),
#ifdef Q_OS_MAC
        QStringLiteral("%1/../Resources/translations").arg(QCoreApplication::applicationDirPath()),
#endif // Q_OS_MAC
    };
}
