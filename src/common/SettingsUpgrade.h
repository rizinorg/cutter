#ifndef COMMON_SETTINGS_UPGRADE_H
#define COMMON_SETTINGS_UPGRADE_H

#include <QSettings>
#include <core/Cutter.h>

namespace Cutter {
    void initializeSettings();
    void migrateThemes();
}

#endif // COMMON_SETTINGS_UPGRADE_H
