#include "Configuration.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QFontDatabase>
#include <QFile>
#include <QApplication>

Configuration* Configuration::mPtr = nullptr;

Configuration::Configuration() : QObject()
{
    mPtr = this;
    loadInitial();
}

Configuration* Configuration::instance()
{
    if (!mPtr)
        mPtr = new Configuration();
    return mPtr;
}

void Configuration::loadInitial()
{
    setDarkTheme(getDarkTheme());
    setColorTheme(getCurrentTheme());
}

void Configuration::resetAll()
{
    Core()->cmd("e-");
    Core()->setSettings();
    Core()->resetDefaultAsmOptions();
    // Delete the file so no extra configuration is in it.
    QFile settingsFile(s.fileName());
    settingsFile.remove();
    s.clear();

    loadInitial();
    emit fontsUpdated();
    Core()->triggerAsmOptionsChanged();
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
    // Windows background
    setColor("gui.background", QColor(255, 255, 255));
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

void Configuration::loadDarkTheme()
{
    /* Load Qt Theme */
    QFile f(":qdarkstyle/style.qss");
    if (!f.exists())
    {
        qWarning() << "Can't find dark theme stylesheet.";
    }
    else
    {
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
    setColor("gui.border",  QColor(255, 255, 255));
    setColor("highlight", QColor(64, 115, 115));
    // Windows background
    setColor("gui.background", QColor(36, 66, 79));
    // Disassembly nodes background
    setColor("gui.alt_background", QColor(58, 100, 128));
    // Custom
    setColor("gui.imports", QColor(50, 140, 255));
    setColor("gui.main", QColor(0, 128, 0));
    setColor("gui.navbar.err", QColor(255, 0, 0));
    setColor("gui.navbar.code", QColor(104, 229, 69));
    setColor("gui.navbar.str", QColor(69, 104, 229));
    setColor("gui.navbar.sym", QColor(229, 150, 69));
    setColor("gui.navbar.empty", QColor(100, 100, 100));
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

void Configuration::setDarkTheme(bool set)
{
    s.setValue("dark", set);
    if (set) {
        loadDarkTheme();
    } else {
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
