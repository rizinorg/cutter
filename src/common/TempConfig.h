
#ifndef TEMPCONFIG_H
#define TEMPCONFIG_H

#include "core/CutterCommon.h"

#include <QString>
#include <QVariant>

/**
 * @brief Class for temporary modifying Rizin `e` configuration.
 *
 * Modified values will be restored at the end of scope. This is useful when using a Rizin command
 * that can only be configured using `e` configuration and doesn't accept arguments. TempConfig::set
 * calls can be chained. If a command or Rizin method accepts arguments directly it is preferred to
 * use those instead of temporary modifying global configuration.
 *
 * \code
 * {
 *     TempConfig tempConfig;
 *     tempConfig.set("asm.arch", "x86").set("asm.comments", false);
 *     // config automatically restored at the end of scope
 * }
 * \endcode
 */
class CUTTER_EXPORT TempConfig
{
public:
    TempConfig() = default;
    ~TempConfig();

    TempConfig &set(const QString &key, const QString &value);
    TempConfig &set(const QString &key, const char *value);
    TempConfig &set(const QString &key, int value);
    TempConfig &set(const QString &key, bool value);

private:
    TempConfig(const TempConfig &) = delete;
    TempConfig &operator=(const TempConfig &) = delete;
    QMap<QString, QVariant> resetValues;
};

#endif // TEMPCONFIG_H
