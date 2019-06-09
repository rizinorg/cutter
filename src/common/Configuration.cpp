#include "Configuration.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFontDatabase>
#include <QFile>
#include <QApplication>
#include <QLibraryInfo>

#include "common/ColorThemeWorker.h"

/* Map with names of themes associated with its color palette
 * (Dark or Light), so for dark interface themes will be shown only Dark color themes
 * and for light - only light ones.
 */
const QHash<QString, ColorFlags> Configuration::relevantThemes = {
    { "ayu", DarkFlag },
    { "consonance", DarkFlag },
    { "darkda", DarkFlag },
    { "onedark", DarkFlag },
    { "solarized", DarkFlag },
    { "zenburn", DarkFlag },
    { "cutter", LightFlag },
    { "dark", LightFlag },
    { "matrix", LightFlag },
    { "tango", LightFlag },
    { "white", LightFlag }
};

const QHash<QString, QHash<ColorFlags, QColor>> Configuration::cutterOptionColors = {
    { "gui.cflow",                 { { DarkFlag,  QColor(0xff, 0xff, 0xff) },
                                     { LightFlag, QColor(0x00, 0x00, 0x00) }} },
    { "gui.dataoffset",            { { DarkFlag,  QColor(0xff, 0xff, 0xff) },
                                     { LightFlag, QColor(0x00, 0x00, 0x00) }} },
    { "gui.imports",               { { DarkFlag,  QColor(50, 140, 255) },
                                     { LightFlag, QColor(50, 140, 255) }} },
    { "gui.item_invalid",          { { DarkFlag,  QColor(155, 155, 155) },
                                     { LightFlag, QColor(155, 155, 155) }} },
    { "gui.main",                  { { DarkFlag,  QColor(0, 128, 0) },
                                     { LightFlag, QColor(0, 128, 0) }} },
    { "gui.item_unsafe",           { { DarkFlag,  QColor(255, 129, 123) },
                                     { LightFlag, QColor(255, 129, 123) }} },
    { "gui.navbar.seek",           { { DarkFlag,  QColor(233, 86, 86) },
                                     { LightFlag, QColor(255, 0, 0) }} },
    { "gui.navbar.pc",             { { DarkFlag,  QColor(66, 238, 244) },
                                     { LightFlag, QColor(66, 238, 244) }} },
    { "gui.navbar.code",           { { DarkFlag,  QColor(130, 200, 111) },
                                     { LightFlag, QColor(104, 229, 69) }} },
    { "gui.navbar.str",            { { DarkFlag,  QColor(111, 134, 216) },
                                     { LightFlag, QColor(69, 104, 229) }} },
    { "gui.navbar.sym",            { { DarkFlag,  QColor(221, 163, 104) },
                                     { LightFlag, QColor(229, 150, 69) }} },
    { "gui.navbar.empty",          { { DarkFlag,  QColor(100, 100, 100) },
                                     { LightFlag, QColor(220, 236, 245) }} },
    { "gui.breakpoint_background", { { DarkFlag,  QColor(140, 76, 76) },
                                     { LightFlag, QColor(233, 143, 143) }} },
    { "gui.overview.node",         { { DarkFlag,  QColor(100, 100, 100) },
                                     { LightFlag, QColor(245, 250, 255) }} },
    { "gui.tooltip.background",    { { DarkFlag,  QColor(42, 44, 46) },
                                     { LightFlag, QColor(250, 252, 254) }} },
    { "gui.tooltip.foreground",    { { DarkFlag,  QColor(250, 252, 254) },
                                     { LightFlag, QColor(42, 44, 46) }} },
    { "gui.border",                { { DarkFlag,  QColor(100, 100, 100) },
                                     { LightFlag, QColor(145, 200, 250) }} },
    { "gui.background",            { { DarkFlag,  QColor(37, 40, 43) },
                                     { LightFlag, QColor(255, 255, 255) }} },
    { "gui.alt_background",        { { DarkFlag,  QColor(28, 31, 36) },
                                     { LightFlag, QColor(245, 250, 255) }} },
    { "gui.disass_selected",       { { DarkFlag,  QColor(31, 34, 40) },
                                     { LightFlag, QColor(255, 255, 255) }} },
    { "lineHighlight",             { { DarkFlag,  QColor(21, 29, 29, 150) },
                                     { LightFlag, QColor(210, 210, 255, 150) }} },
    { "wordHighlight",             { { DarkFlag,  QColor(52, 58, 71, 255) },
                                     { LightFlag, QColor(179, 119, 214, 60) }} },
    { "highlightPC",               { { DarkFlag,  QColor(87, 26, 7) },
                                     { LightFlag, QColor(214, 255, 210) }} },
    { "gui.overview.fill",         { { DarkFlag,  QColor(255, 255, 255, 40) },
                                     { LightFlag, QColor(175, 217, 234, 65) }} },
    { "gui.overview.border",       { { DarkFlag,  QColor(99, 218, 232, 50) },
                                     { LightFlag, QColor(99, 218, 232, 50) }} },
    { "gui.navbar.err",            { { DarkFlag,  QColor(3, 170, 245) },
                                     { LightFlag, QColor(3, 170, 245) }} }
};

Configuration *Configuration::mPtr = nullptr;

/**
 * @brief All asm.* options saved as settings. Values are the default values.
 */
static const QHash<QString, QVariant> asmOptions = {
    { "asm.esil",           false },
    { "asm.pseudo",         false },
    { "asm.offset",         true },
    { "asm.xrefs",          false },
    { "asm.indent",         false },
    { "asm.describe",       false },
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
    { "asm.bb.line",         false },
    { "asm.capitalize",     false },
    { "asm.var.sub",        true },
    { "asm.var.subonly",    true },
    { "asm.tabs",           5 },
    { "asm.tabs.off",       5 },
    { "asm.marks",          false },
    { "esil.breakoninvalid",   true },
    { "graph.offset",       false}
};


Configuration::Configuration() : QObject(), nativePalette(qApp->palette())
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
    setInterfaceTheme(getInterfaceTheme());
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

bool Configuration::getAutoUpdateEnabled() const
{
    return s.value("autoUpdateEnabled", false).toBool();
}

void Configuration::setAutoUpdateEnabled(bool au)
{
    s.setValue("autoUpdateEnabled", au);
}

/**
 * @brief get the current Locale set in Cutter's user configuration
 * @return a QLocale object describes user's current locale
 */
QLocale Configuration::getCurrLocale() const
{
    return s.value("locale", QLocale().system()).toLocale();
}

/**
 * @brief sets Cutter's locale
 * @param l - a QLocale object describes the locate to configure
 */
void Configuration::setLocale(const QLocale &l)
{
    s.setValue("locale", l);
}

/**
 * @brief set Cutter's interface language by a given locale name
 * @param language - a string represents the name of a locale language
 * @return true on success
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
    return nativeWindowIsDark();
}

bool Configuration::nativeWindowIsDark()
{
    const QPalette &palette = qApp->palette();
    auto windowColor = palette.color(QPalette::Window).toRgb();
    return (windowColor.red() + windowColor.green() + windowColor.blue()) < 382;
}

void Configuration::loadNativeStylesheet()
{
    /* Load Qt Theme */
    QFile f(":native/native.qss");
    if (!f.exists()) {
        qWarning() << "Can't find Native theme stylesheet.";
    } else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        QString stylesheet = ts.readAll();
        qApp->setStyleSheet(stylesheet);
    }

    qApp->setPalette(nativePalette);
    /* Some widgets does not change its palette when QApplication changes one
     * so this loop force all widgets do this, but all widgets take palette from
     * QApplication::palette() when they are created so line above is necessary too.
     */
    for (auto widget : qApp->allWidgets()) {
        widget->setPalette(nativePalette);
    }
}

/**
 * @brief Loads the Light theme of Cutter and modify special theme colors
 */
void Configuration::loadLightStylesheet()
{
    /* Load Qt Theme */
    QFile f(":lightstyle/light.qss");
    if (!f.exists()) {
        qWarning() << "Can't find Light theme stylesheet.";
    } else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        QString stylesheet = ts.readAll();

        QPalette p = qApp->palette();
        p.setColor(QPalette::Text, Qt::black);
        qApp->setPalette(p);

        qApp->setStyleSheet(stylesheet);
    }
}

void Configuration::loadDarkStylesheet()
{
    /* Load Qt Theme */
    QFile f(":qdarkstyle/style.qss");
    if (!f.exists()) {
        qWarning() << "Can't find Dark theme stylesheet.";
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
        QPalette p = qApp->palette();
        p.setColor(QPalette::Text, Qt::white);
        qApp->setPalette(p);
        qApp->setStyleSheet(stylesheet);
    }
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

QString Configuration::getLastThemeOf(const CutterInterfaceTheme &currInterfaceTheme) const
{
    return s.value("lastThemeOf." + currInterfaceTheme.name,
                   Config()->getColorTheme()).toString();
}

void Configuration::setInterfaceTheme(int theme)
{
    if (theme >= cutterInterfaceThemesList().size() ||
            theme < 0) {
        theme = 0;
    }
    s.setValue("ColorPalette", theme);

    CutterInterfaceTheme interfaceTheme = cutterInterfaceThemesList()[theme];

    if (interfaceTheme.name == "Native") {
        loadNativeStylesheet();
    } else if (interfaceTheme.name == "Dark") {
        loadDarkStylesheet();
    } else if (interfaceTheme.name == "Light") {
        loadLightStylesheet();
    } else {
        loadNativeStylesheet();
    }

    for (auto it = cutterOptionColors.cbegin(); it != cutterOptionColors.cend(); it++) {
        setColor(it.key(), it.value()[interfaceTheme.flag]);
    }

    emit interfaceThemeChanged();
    emit colorsUpdated();
}

const CutterInterfaceTheme *Configuration::getCurrentTheme()
{
    int i = getInterfaceTheme();
    if (i < 0 || i >= cutterInterfaceThemesList().size()) {
        i = 0;
        setInterfaceTheme(i);
    }
    return &cutterInterfaceThemesList()[i];
}

QString Configuration::getLogoFile()
{
    return windowColorIsDark()
           ? QString(":/img/cutter_white_plain.svg")
           : QString(":/img/cutter_plain.svg");
}

/**
 * @brief Configuration::setColor sets the local Cutter configuration color
 * @param name Color Name
 * @param color The color you want to set
 */
void Configuration::setColor(const QString &name, const QColor &color)
{
    s.setValue("colors." + name, color);
}

void Configuration::setLastThemeOf(const CutterInterfaceTheme &currInterfaceTheme, const QString &theme)
{
    s.setValue("lastThemeOf." + currInterfaceTheme.name, theme);
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

    QJsonObject colorTheme = ThemeWorker().getTheme(theme).object();
    for (auto it = colorTheme.constBegin(); it != colorTheme.constEnd(); it++) {
        QJsonArray rgb = it.value().toArray();
        if (rgb.size() != 4) {
            continue;
        }
        setColor(it.key(), QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt(), rgb[3].toInt()));
    }

    // Trick Cutter to load colors that are not specified in standard theme
    if (!ThemeWorker().isCustomTheme(theme)) {
        setInterfaceTheme(getInterfaceTheme());
        return;
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

const QList<CutterInterfaceTheme>& Configuration::cutterInterfaceThemesList()
{
    static const QList<CutterInterfaceTheme> list = {
        { "Native", Configuration::nativeWindowIsDark() ? DarkFlag : LightFlag },
        { "Dark",   DarkFlag },
        { "Light",  LightFlag }
    };
    return list;
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

/**
 * @brief this function will gather and return available translation for Cutter
 * @return a list of all available translations
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

/**
 * @brief check if this is the first time Cutter's is executed on this computer
 * @return true if this is first execution; otherwise returns false.
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
