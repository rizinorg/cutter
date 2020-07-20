
#ifndef TEMPCONFIG_H
#define TEMPCONFIG_H

#include <QString>
#include <QVariant>

/**
 * @brief Class for temporary modifying r2 `e` configuration.
 *
 * Modified values will be restored at the end of scope. This is useful when using a r2 command that can only
 * be configured using `e` configuration and doesn't accept arguments. TempConfig::set calls can be chained.
 * If a command or r2 method accepts arguments directly it is preferred to use those instead of temporary modifying
 * global configuration.
 *
 * \code
 * {
 *     TempConfig tempConfig;
 *     tempConfig.set("asm.arch", "x86").set("asm.comments", false);
 *     return Core()->cmdRaw("pd");
 *     // config automatically restored at the end of scope
 * }
 * \endcode
 */
class TempConfig
{
public:
    TempConfig() = default;
    ~TempConfig();

    TempConfig &set(const QString &key, const QString &value);
    TempConfig &set(const QString &key, const char *value);
    TempConfig &set(const QString &key, int value);
    TempConfig &set(const QString &key, bool value);

private:
    TempConfig(const TempConfig&) = delete;
    TempConfig &operator=(const TempConfig&) = delete;
    QMap<QString, QVariant> resetValues;
};

#endif //TEMPCONFIG_H
