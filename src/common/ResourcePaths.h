#ifndef RESOURCE_PATHS_H
#define RESOURCE_PATHS_H

#include <QStandardPaths>

/**
 * \file ResourcePaths.h
 * Set of functions for obtaining various paths. Some of the functions are wrappers around
 * QStandardPaths functions having the same name but with modifications specific to way
 * Cutter is packaged.
 */

namespace Cutter {
QStringList locateAll(
    QStandardPaths::StandardLocation type,
    const QString &fileName,
    QStandardPaths::LocateOptions options = QStandardPaths::LocateFile);
QStringList standardLocations(QStandardPaths::StandardLocation type);
QString writableLocation(QStandardPaths::StandardLocation type);


/**
 * @brief Get list of available translation directories (depends on configuration and OS)
 * @return list of directories
 */
QStringList getTranslationsDirectories();
}

#endif
