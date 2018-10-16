#ifndef ABSTRACTOPTIONWIDGET_H
#define ABSTRACTOPTIONWIDGET_H

#include <QDialog>
#include <QJsonObject>

namespace AbstractOptionWidgetNamespace {
class Settings : public QJsonObject
{
public:
    Settings();
    ~Settings();
    void setValue(const QString &key, const QVariant &data);
    QVariant value(const QString &key);
};
}

class AbstractOptionWidget : public QDialog
{
    Q_OBJECT
public:
    AbstractOptionWidget(QWidget *parent = nullptr);
    ~AbstractOptionWidget() {}

    virtual void apply() = 0;
    virtual void discard() = 0;

    bool getIsChanged() const;

protected:
    AbstractOptionWidgetNamespace::Settings currSettings;
    bool isChanged;
};

#endif // ABSTRACTOPTIONWIDGET_H
