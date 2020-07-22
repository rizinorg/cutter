#include "common/ResourcePaths.h"

#include <QLibraryInfo>
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QDebug>


#ifdef APPIMAGE
static QDir appimageRoot()
{
    auto dir = QDir(QCoreApplication::applicationDirPath()); // appimage/usr/bin
    dir.cdUp(); // /usr
    dir.cdUp(); // /
    return dir;
}
#endif

static QString substitutePath(QString path)
{
#ifdef APPIMAGE
    if (path.startsWith("/usr")) {
        return appimageRoot().absoluteFilePath("." + path);
    }
#endif
    return path;
}

/**
 * @brief Substitute or filter paths returned by standardLocations based on Cutter package kind.
 * @param paths list of paths to process
 * @return
 */
static QStringList substitutePaths(const QStringList &paths)
{
    QStringList result;
    result.reserve(paths.size());
    for (auto &path : paths) {
        // consider ignoring some of system folders for portable packages here or standardLocations if it depends on path type
        result.push_back(substitutePath(path));
    }
    return result;
}

QStringList Cutter::locateAll(QStandardPaths::StandardLocation type, const QString &fileName,
                              QStandardPaths::LocateOptions options)
{
    // This function is reimplemented here instead of forwarded to Qt becauase existence check needs to be done
    // after substitutions
    QStringList result;
    for (auto path : standardLocations(type)) {
        QString filePath = path + QLatin1Char('/') + fileName;
        bool exists = false;
        if (options & QStandardPaths::LocateDirectory) {
            exists = QDir(filePath).exists();
        } else {
            exists = QFileInfo(filePath).isFile();
        }
        if (exists) {
            result.append(filePath);
        }
    }
    return result;
}

QStringList Cutter::standardLocations(QStandardPaths::StandardLocation type)
{
    return substitutePaths(QStandardPaths::standardLocations(type));
}

QString Cutter::writableLocation(QStandardPaths::StandardLocation type)
{
    return substitutePath(QStandardPaths::writableLocation(type));
}

QStringList Cutter::getTranslationsDirectories()
{
    auto result = locateAll(QStandardPaths::DataLocation, "translations",
                            QStandardPaths::LocateDirectory);
    result << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    return result;
}

