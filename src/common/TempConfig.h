
#ifndef TEMPCONFIG_H
#define TEMPCONFIG_H

#include <QString>
#include <QVariant>

class TempConfig
{
public:
    TempConfig() = default;
    ~TempConfig();

    TempConfig &set(const QString &key, const QString &value);
    TempConfig &set(const QString &key, int value);
    TempConfig &set(const QString &key, bool value);

private:
    QMap<QString, QVariant> resetValues;
};

#endif //TEMPCONFIG_H
