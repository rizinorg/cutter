#include "Configuration.h"
#include <QJsonObject>
#include <QJsonArray>

Configuration* Configuration::mPtr = nullptr;

Configuration::Configuration() : QObject()
{
    mPtr = this;

    loadDefaultColors();
}

Configuration* Configuration::instance()
{
    return mPtr;
}

void Configuration::loadDefaultColors()
{
    //Core()->cmd("eco behelit");
    Core()->cmd("eco smyck");
    QJsonObject colors = Core()->cmdj("ecj").object();
    for (auto color : colors.keys()) {
        QJsonArray rgb = colors[color].toArray();
        QColor col = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
        s.setValue("colors." + color, col);
    }
    //s.setValue("colors.gui.alt_background", QColor(255, 255, 255));
}

const QFont Configuration::getFont() const
{
    QFont font = s.value("font", QFont("Monospace", 12)).value<QFont>();
    return font;
}

void Configuration::setFont(const QFont &font)
{
    s.setValue("font", font);
    emit fontsUpdated();
}

const QColor Configuration::getColor(const QString &name) const
{
    if (s.contains("colors." + name)) {
        return s.value("colors." + name).value<QColor>();
    } else {
        return s.value("colors.other").value<QColor>();
    }
}
