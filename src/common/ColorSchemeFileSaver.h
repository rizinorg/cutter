#ifndef COLORSCHEMEFILESAVER_H
#define COLORSCHEMEFILESAVER_H

#include <QDir>
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

    /**
     * @brief Imports file @a srcScheme as a scheme with name of file.
     * @param Full path to Cutter scheme, where name of file is name of scheme.
     * @return "" on succes or error string.
     */
    QString importScheme(const QString &srcScheme) const;

    /**
     * @brief Exports @a srcScheme color scheme to @a destFile file.
     * @param srcScheme
     * Name of scheme to be exported. It can be either standard or custom one.
     * @param destFile
     * Full path to file where scheme will be imported.
     * @return "" on success or error string.
     */
    QString exportScheme(const QString &srcScheme, const QString& destFile) const;

    /**
     * @brief Checks whether or not @a schemeName is a scheme created by user
     * and not default radare2 scheme.
     * @param schemeName
     * Name of scheme to be checked.
     * @return True if @a schemeName is custom scheme
     * otherwise false
     * @note
     * If there are no scheme named @a schemeName, function will return false.
     */
    bool isCustomScheme(const QString &schemeName) const;

    /**
     * @brief Renames @a schemeName scheme to @a newName.
     * @param schemeName
     * Name of scheme to be renamed.
     * @param newName
     * Name of scheme after rename.
     * @return "" on success or error string on failure
     */
    QString rename(const QString &schemeName, const QString &newName) const;

    bool isNameEngaged(const QString &name) const;

    QMap<QString, QColor> getCutterSpecific() const;

    QStringList getCustomSchemes() const;

    void deleteScheme(const QString &schemeName) const;

private:
    /**
     * @brief Checks whether or not @a file is Cutter color scheme file
     * @param file
     * Path to file to be checked.
     * @param output
     * Output of the check. True if @file is Cutter color scheme
     * otherwise false.
     * @return false if @a file can not be opened otherwise true
     */
    bool isSchemeFile(const QString &file, bool* output) const;

    QString standardR2ThemesLocationPath;
    QString customR2ThemesLocationPath;

    ColorSchemeFileSaver(QObject *parent = nullptr);
    ColorSchemeFileSaver(const ColorSchemeFileSaver &root) = delete;
    ColorSchemeFileSaver &operator=(const ColorSchemeFileSaver &) = delete;
};

#endif // COLORSCHEMEFILESAVER_H
