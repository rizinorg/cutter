#include "TempConfig.h"

#include "core/Cutter.h"

#include <cassert>

TempConfig::~TempConfig()
{
    for (auto i = resetValues.constBegin(); i != resetValues.constEnd(); ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        switch (i.value().typeId()) {
        case QMetaType::Type::QString:
            Core()->setConfig(i.key(), i.value().toString());
            break;
        case QMetaType::Type::Int:
            Core()->setConfig(i.key(), i.value().toInt());
            break;
        case QMetaType::Type::Bool:
            Core()->setConfig(i.key(), i.value().toBool());
            break;
        default:
            assert(false);
            break;
        }
#else
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
#endif
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
