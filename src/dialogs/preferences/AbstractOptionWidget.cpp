#include "dialogs/preferences/AbstractOptionWidget.h"
#include <QVariant>

AbstractOptionWidget::AbstractOptionWidget(QWidget *parent) : QDialog(parent),
    isChanged(false)
{

}

bool AbstractOptionWidget::getIsChanged() const
{
    return isChanged;
}

AbstractOptionWidgetNamespace::Settings::Settings() : QJsonObject()
{
}

AbstractOptionWidgetNamespace::Settings::~Settings()
{
}

void AbstractOptionWidgetNamespace::Settings::setValue(const QString &key, const QVariant &data)
{
    insert(key, QJsonValue::fromVariant(data));
}

QVariant AbstractOptionWidgetNamespace::Settings::value(const QString &key)
{
    return QJsonObject::value(key).toVariant();
}
