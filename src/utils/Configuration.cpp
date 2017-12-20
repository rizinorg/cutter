#include "Configuration.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QFontDatabase>

Configuration* Configuration::mPtr = nullptr;

Configuration::Configuration() : QObject()
{
    mPtr = this;
    loadInitial();
}

Configuration* Configuration::instance()
{
    return mPtr;
}

void Configuration::loadInitial()
{
    setDarkTheme(getDarkTheme());
}

void Configuration::resetAll()
{
    s.clear();
    Core()->cmd("e-");
    Core()->setSettings();
    Core()->resetDefaultAsmOptions();

    loadInitial();
    emit fontsUpdated();
    Core()->triggerAsmOptionsChanged();
}

void Configuration::loadDefaultTheme()
{
    QColor color0 = QColor(0, 0, 0);
    QColor color1 = QColor(0, 0, 255);
    QColor color2 = QColor(0, 128, 0);
    QColor color3 = QColor(255, 0, 0);
    QColor color4 = QColor(127, 0, 127);
    QColor color5 = QColor(95, 95, 175);
    QColor color6 = QColor(255, 235, 95);
    QColor color7 = QColor(215, 135, 0);
    QColor color8 = QColor(108, 108, 108);
    QColor color9 = QColor(96, 48, 0);
    QColor colorA = QColor(50, 140, 255);

    QColor highlightColor = QColor(210, 210, 255);

    // Instructions
    setColor("comment",     color4);
    setColor("usrcmt",      color2);
    setColor("args",        color5);
    setColor("fname",       color1);
    setColor("floc",        color6);
    setColor("fline",       color2);
    setColor("flag",        color1);
    setColor("label",       color7);
    setColor("help",        color1);
    setColor("flow",        color2);
    setColor("flow2",       color2);
    setColor("prompt",      color0);
    setColor("offset",      color0);
    setColor("input",       color0);
    setColor("invalid",     color3);
    setColor("other",       color1);
    setColor("b0x00",       color8);
    setColor("b0x7f",       color0);
    setColor("b0xff",       color3);
    setColor("math",        color1);
    setColor("bin",         color1);
    setColor("btext",       color0);
    setColor("push",        color0);
    setColor("pop",         color0);
    setColor("crypto",      color4);
    setColor("jmp",         color2);
    setColor("cjmp",        color2);
    setColor("call",        color9);
    setColor("nop",         color1);
    setColor("ret",         color9);
    setColor("trap",        color3);
    setColor("swi",         color2);
    setColor("cmp",         color0);
    setColor("reg",         color1);
    setColor("creg",        color1);
    setColor("num",         color1);
    setColor("mov",         color0);

    // AI
    setColor("ai.read",     color0);
    setColor("ai.write",    color0);
    setColor("ai.exec",     color0);
    setColor("ai.seq",      color0);
    setColor("ai.ascii",    color0);

    // Graphs
    setColor("graph.box",   color0);
    setColor("graph.box2",  color2);
    setColor("graph.box3",  color3);
    setColor("graph.box4",  color0);
    setColor("graph.true",  color2);
    setColor("graph.false", color3);
    setColor("graph.trufae", color0);
    setColor("graph.current", color0);
    setColor("graph.traced", color3);

    // GUI
    setColor("gui.cflow",   color0);
    setColor("gui.dataoffset", color0);
    setColor("gui.border",  color0);
    setColor("highlight",   highlightColor);
    // Windows background
    setColor("gui.background", QColor(255, 255, 255));
    // Disassembly nodes background
    setColor("gui.alt_background", QColor(245, 250, 255));
    // Custom
    setColor("gui.imports", colorA);

}

void Configuration::loadDarkTheme()
{
    QColor color0 = QColor(255, 255, 255);
    QColor color1 = QColor(100, 180, 255);
    QColor color2 = QColor(0, 255, 0);
    QColor color3 = QColor(255, 0, 0);
    QColor color4 = QColor(128, 235, 200);
    QColor color5 = QColor(95, 95, 175);
    QColor color6 = QColor(255, 235, 95);
    QColor color7 = QColor(255, 130, 0);
    QColor color8 = QColor(108, 108, 108);
    QColor color9 = QColor(255, 130, 0);
    QColor colorA = QColor(50, 140, 255);

    QColor highlightColor = QColor(64, 115, 115);

    // Instructions
    setColor("comment",     color4);
    setColor("usrcmt",      color2);
    setColor("args",        color5);
    setColor("fname",       color1);
    setColor("floc",        color6);
    setColor("fline",       color2);
    setColor("flag",        color1);
    setColor("label",       color7);
    setColor("help",        color1);
    setColor("flow",        color2);
    setColor("flow2",       color2);
    setColor("prompt",      color0);
    setColor("offset",      color0);
    setColor("input",       color0);
    setColor("invalid",     color3);
    setColor("other",       color1);
    setColor("b0x00",       color8);
    setColor("b0x7f",       color0);
    setColor("b0xff",       color3);
    setColor("math",        color1);
    setColor("bin",         color1);
    setColor("btext",       color0);
    setColor("push",        color0);
    setColor("pop",         color0);
    setColor("crypto",      color4);
    setColor("jmp",         color2);
    setColor("cjmp",        color2);
    setColor("call",        color9);
    setColor("nop",         color1);
    setColor("ret",         color9);
    setColor("trap",        color3);
    setColor("swi",         color2);
    setColor("cmp",         color0);
    setColor("reg",         color1);
    setColor("creg",        color1);
    setColor("num",         color1);
    setColor("mov",         color0);

    // AI
    setColor("ai.read",     color0);
    setColor("ai.write",    color0);
    setColor("ai.exec",     color0);
    setColor("ai.seq",      color0);
    setColor("ai.ascii",    color0);

    // Graphs
    setColor("graph.box",   color0);
    setColor("graph.box2",  color2);
    setColor("graph.box3",  color3);
    setColor("graph.box4",  color0);
    setColor("graph.true",  color2);
    setColor("graph.false", color3);
    setColor("graph.trufae", color0);
    setColor("graph.current", color0);
    setColor("graph.traced", color3);

    // GUI
    setColor("gui.cflow",   color0);
    setColor("gui.dataoffset", color0);
    setColor("gui.border",  color0);
    setColor("highlight", highlightColor);
    // Windows background
    setColor("gui.background", QColor(36, 66, 79));
    // Disassembly nodes background
    setColor("gui.alt_background", QColor(58, 100, 128));
    // Custom
    setColor("gui.imports", colorA);
}

const QFont Configuration::getFont() const
{
    QFont font = s.value("font", QFontDatabase::systemFont(QFontDatabase::FixedFont)).value<QFont>();
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

const QColor Configuration::getColor(const QString &name) const
{
    if (s.contains("colors." + name)) {
        return s.value("colors." + name).value<QColor>();
    } else {
        return s.value("colors.other").value<QColor>();
    }
}

/**
 * @brief Configuration::setColor sets the local Cutter configuration color
 * and the radare2 color.
 * @param name Color Name
 * @param color The color you want to set
 */
void Configuration::setColor(const QString &name, const QColor &color)
{
    s.setValue("colors." + name, color);
    // R2 does not support truecolor properly
    QString col = QString("rgb:%1%2%3").arg(color.red() >> 4, 1, 16).arg(color.green() >> 4, 1, 16).arg(color.blue() >> 4, 1, 16);
    // Not clean but this is a private function so name should NEVER be a bad input
    Core()->cmd("ec " + name + " " + col);
}
