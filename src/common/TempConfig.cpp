
#include <cassert>

#include "core/Cutter.h"
#include "TempConfig.h"

TempConfig::~TempConfig()
{
    for (auto i = resetValues.constBegin(); i != resetValues.constEnd(); ++i) {
        switch (i.value().type()) {
        case QVariant::String:
            Core()->setConfig(i.key(), i.value().toString());
            break;
        case QVariant::Int:
            Core()->setConfig(i.key(), i.value().toInt());
            break;
        case QVariant::Bool:
            Core()->setConfig(i.key(), i.value().toBool());
            break;
        default:
            assert(false);
            break;
        }
    }
}

TempConfig &TempConfig::set(const QString &key, const QString &value)
{
    if (!resetValues.contains(key)) {
        resetValues[key] = Core()->getConfig(key);
    }

    Core()->setConfig(key, value);
    return *this;
}

TempConfig &TempConfig::set(const QString &key, const char *value)
{
    if (!resetValues.contains(key)) {
        resetValues[key] = Core()->getConfig(key);
    }

    Core()->setConfig(key, value);
    return *this;
}

TempConfig &TempConfig::set(const QString &key, int value)
{
    if (!resetValues.contains(key)) {
        resetValues[key] = Core()->getConfigi(key);
    }

    Core()->setConfig(key, value);
    return *this;
}

TempConfig &TempConfig::set(const QString &key, bool value)
{
    if (!resetValues.contains(key)) {
        resetValues[key] = Core()->getConfigb(key);
    }

    Core()->setConfig(key, value);
    return *this;
}
