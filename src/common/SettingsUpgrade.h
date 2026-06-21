#ifndef SETTINGS_UPGRADE_H
#define SETTINGS_UPGRADE_H

#include <QSettings>

#include <core/Cutter.h>

/**
 * @file SettingsUpgrade.h
 * @brief Logic for migrating and importing Cutter settings from older versions
 */

namespace Cutter {
void initializeSettings();
/**
 * @brief Check if Cutter should offer importing settings from version that can't be directly
 * updated.
 * @return True if this is first time running Cutter and r2 based Cutter <= 1.12 settings exist.
 */
bool shouldOfferSettingImport();
/**
 * @brief Ask user if Cutter should import settings from pre-rizin Cutter.
 *
 * This function assume that QApplication isn't running yet.
 */
void showSettingImportDialog(int &argc, char **argv);
void migrateThemes();
}

#endif // SETTINGS_UPGRADE_H
