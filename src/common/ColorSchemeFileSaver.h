#ifndef COLORSCHEMEFILESAVER_H
#define COLORSCHEMEFILESAVER_H

#include <QFile>
#include <QColor>
#include <QObject>

constexpr const char *standardBackgroundOptionName = "gui.background";
constexpr const char *standardTextOptionName = "btext";
#define ColorSchemeFileWorker() (ColorSchemeFileSaver::instance())

class ColorSchemeFileSaver : public QObject
{
    Q_OBJECT
public:
    static ColorSchemeFileSaver &instance()
    {
        static ColorSchemeFileSaver ex;
        return ex;
    }

    virtual ~ColorSchemeFileSaver() {}


    /**
     * @brief Copies @a srcSchemeName with name @a copySchemeName.
     * @param srcSchemeName
     * Name of scheme to be copied.
     * @param copySchemeName
     * Name of copy.
     * @return "" on success or error message.
     */
    QString copy(const QString &srcSchemeName, const QString &copySchemeName) const;

    QString save(const QJsonDocument& scheme, const QString &schemeName) const;

    bool isCustomScheme(const QString &schemeName) const;

    bool isSchemeExist(const QString &name) const;

    /**
     * @brief Returns colors for color options used in Cutter, but not
     * in radare2 (such as navbar colors, breakpoint colors, etc).
     */
    QMap<QString, QColor> getCutterSpecific() const;

    QStringList getCustomSchemes() const;

    void deleteScheme(const QString &schemeName) const;

private:
    QString standardR2ThemesLocationPath;
    QString customR2ThemesLocationPath;

    ColorSchemeFileSaver(QObject *parent = nullptr);
    ColorSchemeFileSaver(const ColorSchemeFileSaver &root) = delete;
    ColorSchemeFileSaver &operator=(const ColorSchemeFileSaver &) = delete;
};

#endif // COLORSCHEMEFILESAVER_H
