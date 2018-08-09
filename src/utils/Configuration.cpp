#include "Configuration.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QFontDatabase>
#include <QFile>
#include <QApplication>

Configuration *Configuration::mPtr = nullptr;

/*!
 * \brief All asm.* options saved as settings. Values are the default values.
 */
static const QHash<QString, QVariant> asmOptions = {
    { "asm.esil",           false },
    { "asm.pseudo",         false },
    { "asm.offset",         true },
    { "asm.describe",       false },
    { "asm.stackptr",       false },
    { "asm.slow",           true },
    { "asm.lines",          true },
    { "asm.lines.fcn",      true },
    { "asm.flags.offset",   false },
    { "asm.emu",            false },
    { "asm.cmt.right",      true },
    { "asm.cmt.col",        70 },
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
    { "esil.breakoninvalid",   true }
};


Configuration::Configuration() : QObject()
{
    mPtr = this;
    loadInitial();
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
    setColorTheme(getCurrentTheme());
    applySavedAsmOptions();
}

QString Configuration::getDirProjects()
{
    auto projectsDir = s.value("dir.projects").toString();
    if (projectsDir == "") {
        projectsDir = Core()->getConfig("dir.projects");
        setDirProjects(projectsDir);
    }

    return projectsDir;
}

void Configuration::setDirProjects(const QString &dir)
{
    s.setValue("dir.projects", dir);
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

void Configuration::loadDefaultTheme()
{
    /* Load Qt Theme */
    qApp->setStyleSheet("");

    /* Images */
    logoFile = QString(":/img/cutter_plain.svg");

    /* Colors */
    // GUI
    setColor("gui.cflow",   QColor(0, 0, 0));
    setColor("gui.dataoffset", QColor(0, 0, 0));
    setColor("gui.border",  QColor(0, 0, 0));
    setColor("highlight",   QColor(210, 210, 255));
    setColor("highlightWord", QColor(210, 210, 255));
    // RIP line selection in debug
    setColor("highlightPC", QColor(214, 255, 210));
    // Windows background
    setColor("gui.background", QColor(255, 255, 255));
    setColor("gui.disass_selected", QColor(255, 255, 255));
    // Disassembly nodes background
    setColor("gui.alt_background", QColor(245, 250, 255));
    // Custom
    setColor("gui.imports", QColor(50, 140, 255));
    setColor("gui.main", QColor(0, 128, 0));
    setColor("gui.navbar.err", QColor(255, 0, 0));
    setColor("gui.navbar.code", QColor(104, 229, 69));
    setColor("gui.navbar.str", QColor(69, 104, 229));
    setColor("gui.navbar.sym", QColor(229, 150, 69));
    setColor("gui.navbar.empty", QColor(100, 100, 100));
}

void Configuration::loadBaseDark()
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

    /* Images */
    logoFile = QString(":/img/cutter_white_plain.svg");

    /* Colors */
    // GUI
    setColor("gui.cflow",   QColor(255, 255, 255));
    setColor("gui.dataoffset", QColor(255, 255, 255));
    // Custom
    setColor("gui.imports", QColor(50, 140, 255));
    setColor("gui.main", QColor(0, 128, 0));
    setColor("gui.navbar.err", QColor(255, 0, 0));
    setColor("gui.navbar.code", QColor(104, 229, 69));
    setColor("gui.navbar.str", QColor(69, 104, 229));
    setColor("gui.navbar.sym", QColor(229, 150, 69));
    setColor("gui.navbar.empty", QColor(100, 100, 100));
    // RIP line selection in debug
    setColor("highlightPC", QColor(87, 26, 7));
}

void Configuration::loadDarkTheme()
{
    loadBaseDark();
    setColor("gui.border",  QColor(100, 100, 100));
    // Windows background
    setColor("gui.background", QColor(37, 40, 43));
    // Disassembly nodes background
    setColor("gui.alt_background", QColor(28, 31, 36));
    // Disassembly nodes background when selected
    setColor("gui.disass_selected", QColor(44, 53, 54));
    // Disassembly line selected
    setColor("highlight", QColor(21, 29, 29));
    setColor("highlightWord", QColor(100, 100, 100));
}

const QFont Configuration::getFont() const
{
    QFont font = s.value("font", QFont("Inconsolata", 12)).value<QFont>();
    return font;
}

void Configuration::setFont(const QFont &font)
{
    s.setValue("font", font);
    emit fontsUpdated();
}

void Configuration::setTheme(int theme)
{
    s.setValue("ColorPalette", theme);
    switch(theme){
    case 0:
        loadDefaultTheme();
        break;
    case 1:
        loadDarkTheme();
        break;
    default:
        loadDefaultTheme();
    }
    emit colorsUpdated();
}

QString Configuration::getLogoFile()
{
    return logoFile;
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

const QColor Configuration::getColor(const QString &name) const
{
    if (s.contains("colors." + name)) {
        return s.value("colors." + name).value<QColor>();
    } else {
        return s.value("colors.other").value<QColor>();
    }
}

void Configuration::setColorTheme(QString theme)
{
    if (theme == "default") {
        Core()->cmd("ecd");
        s.setValue("theme", "default");
    } else {
        Core()->cmd(QString("eco %1").arg(theme));
        s.setValue("theme", theme);
    }
    // Duplicate interesting colors into our Cutter Settings
    // Dirty fix for arrow colors, TODO refactor getColor, setColor, etc.
    QJsonDocument colors = Core()->cmdj("ecj");
    QJsonObject colorsObject = colors.object();
    QJsonObject::iterator it;
    for (it = colorsObject.begin(); it != colorsObject.end(); it++) {
        if (!it.key().contains("graph"))
            continue;
        QJsonArray rgb = it.value().toArray();
        s.setValue("colors." + it.key(), QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()));
    }
    emit colorsUpdated();
}

void Configuration::resetToDefaultAsmOptions()
{
    for (auto it = asmOptions.begin(); it != asmOptions.end(); it++) {
        setConfig(it.key(), it.value());
    }
}

void Configuration::applySavedAsmOptions()
{
    for (auto it = asmOptions.begin(); it != asmOptions.end(); it++) {
        Core()->setConfig(it.key(), s.value(it.key(), it.value()));
    }
}

QVariant Configuration::getConfigVar(const QString &key)
{
    QHash<QString, QVariant>::const_iterator it = asmOptions.find(key);
    if (it != asmOptions.end()) {
        switch(it.value().type()) {
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
