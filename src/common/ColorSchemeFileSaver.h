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

    QFile::FileError copy(const QString &srcSchemeName, const QString &copySchemeName) const;

    QFile::FileError save(const QString &scheme, const QString &schemeName) const;

    bool isCustomScheme(const QString &schemeName) const;

    bool isNameEngaged(const QString &name) const;

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
