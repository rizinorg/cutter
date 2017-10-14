#include "Configuration.h"

Configuration* Configuration::mPtr = nullptr;

Configuration::Configuration() : QObject()
{
    mPtr = this;
}

Configuration* Configuration::instance()
{
    return mPtr;
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
