#include "SettingsUpgrade.h"

#include "Configuration.h"
#include "common/ColorThemeWorker.h"

#include <QApplication>
#include <QMessageBox>

#define CUTTER_SETTINGS_VERSION_CURRENT 7
#define CUTTER_SETTINGS_VERSION_KEY "version"

#define THEME_VERSION_CURRENT 2
#define THEME_VERSION_KEY "theme_version"

/*
 * How Settings migrations work:
 *
 * Every time settings are changed in a way that needs migration,
 * CUTTER_SETTINGS_VERSION_CURRENT is raised by 1 and a function migrateSettingsToX
 * is implemented and added to initializeSettings().
 * This function takes care of migrating from EXACTLY version X-1 to X.
 */

namespace {

const char preRizinOrg[] = "RadareOrg";
const char preRizinApp[] = "Cutter";
const int lastR2CutterSettingVersion = 6;

/**
 * @brief Migrate Settings used before Cutter 1.8
 *
 * @return whether any settings have been migrated
 */
bool migrateSettingsPre18(QSettings &newSettings)
{
    if (newSettings.value("settings_migrated", false).toBool()) {
        return false;
    }
    QSettings oldSettings(QSettings::NativeFormat, QSettings::Scope::UserScope, "Cutter", "Cutter");
    const QStringList allKeys = oldSettings.allKeys();
    if (allKeys.isEmpty()) {
        return false;
    }
    qInfo() << "Migrating Settings from pre-1.8";
    for (const QString &key : allKeys) {
        newSettings.setValue(key, oldSettings.value(key));
    }
    oldSettings.clear();
    QFile settingsFile(oldSettings.fileName());
    settingsFile.remove();
    newSettings.setValue("settings_migrated", true);
    return true;
}

void migrateSettingsTo1(QSettings &settings)
{
    settings.remove("settings_migrated"); // now handled by version
    settings.remove("updated_custom_themes"); // now handled by theme_version
}

void migrateSettingsTo2(QSettings &settings)
{
    QStringList docks = settings.value("docks").toStringList(); // get current list of docks
    // replace occurences of "PseudocodeWidget" with "DecompilerWidget"
    settings.setValue("docks", docks.replaceInStrings("PseudocodeWidget", "DecompilerWidget"));
}

void migrateSettingsTo3(QSettings &settings)
{
    auto defaultGeometry = settings.value("geometry").toByteArray();
    auto defaultState = settings.value("state").toByteArray();

    auto debugGeometry = settings.value("debug.geometry").toByteArray();
    auto debugState = settings.value("debug.state").toByteArray();

    const auto docks = settings.value("docks", QStringList()).toStringList();
    auto unsyncList = settings.value("unsync", QStringList()).toStringList();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QSet<QString> unsyncDocks = unsyncList.toSet();
#else
    const QSet<QString> unsyncDocks(unsyncList.begin(), unsyncList.end());
#endif

    QVariantMap viewProperties;
    for (auto &dock : docks) {
        QVariantMap properties;
        bool synchronized = true;
        if (unsyncDocks.contains(dock)) {
            synchronized = false;
        }
        properties.insert("synchronized", synchronized);
        viewProperties.insert(dock, properties);
    }

    settings.beginWriteArray("layouts", 2);
    settings.setArrayIndex(0);
    settings.setValue("name", "Default");
    settings.setValue("geometry", defaultGeometry);
    settings.setValue("state", defaultState);
    settings.setValue("docks", viewProperties);

    settings.setArrayIndex(1);
    settings.setValue("name", "Debug");
    settings.setValue("geometry", debugGeometry);
    settings.setValue("state", debugState);
    settings.setValue("docks", viewProperties);

    settings.endArray();

    settings.remove("pos"); // Pos and size already stored within geometry
    settings.remove("size");
    // keep geometry but with slightly different usecase
    settings.remove("state");
    settings.remove("debug.geometry");
    settings.remove("debug.state");
    settings.remove("docks");
    settings.remove("unsync");
}

void renameAsmOption(QSettings &settings, const QString &oldName, const QString &newName)
{
    if (settings.contains(oldName)) {
        auto value = settings.value(oldName);
        settings.remove(oldName);
        settings.setValue(newName, value);
    }
}

void migrateSettingsTo4(QSettings &settings)
{
    renameAsmOption(settings, "asm.var.subonly", "asm.sub.varonly");
    renameAsmOption(settings, "asm.bytespace", "asm.bytes.space");
}

void migrateSettingsTo5(QSettings &settings)
{
    renameAsmOption(settings, "asm.var.sub", "asm.sub.var");
}

void migrateSettingsTo6(QSettings &settings)
{
    settings.remove("dir.projects");
}

void migrateSettingsTo7(QSettings &settings)
{
    auto list = settings.value("recentFileList").toStringList();
    for (auto &file : list) {
        file.prepend("file://");
    }
    settings.setValue("recentFileList", list);
}

void removeObsoleteOptionsFromCustomThemes()
{
    const QStringList options = Core()->getThemeKeys() << ColorThemeWorker::cutterSpecificOptions;
    const QStringList themes = Core()->getColorThemes();
    for (const auto &themeName : themes) {
        if (!ThemeWorker().isCustomTheme(themeName)) {
            continue;
        }
        const ColorThemeWorker::Theme sch = ThemeWorker().getTheme(themeName);
        ColorThemeWorker::Theme updatedTheme;
        for (auto it = sch.constBegin(); it != sch.constEnd(); ++it) {
            if (options.contains(it.key())) {
                updatedTheme.insert(it.key(), it.value());
            }
        }
        ThemeWorker().save(updatedTheme, themeName);
    }
}

void syncCustomThemes()
{
    const QStringList options = Core()->getThemeKeys() << ColorThemeWorker::cutterSpecificOptions;
    const QStringList themes = Core()->getColorThemes();
    const ColorThemeWorker::Theme lightTheme =
            ThemeWorker().getTheme("cutter"); // default light theme
    const ColorThemeWorker::Theme darkTheme = ThemeWorker().getTheme("ayu"); // default dark theme

    // note that there was no entry for angui.navbar.str (as it was a typo) so the
    // default color for it will most likely be black instead of the color defined
    // in config, unless changed by the user
    QHash<QString, QString> renames = {
        { "angui.navbar.str", "gui.navbar.str" },
        { "gui.navbar.empty", "gui.navbar.unexplored" },
    };
    const QStringList forceDefaultKeys = { "gui.navbar.signature", "gui.navbar.data" };

    for (const auto &themeName : themes) {
        if (!ThemeWorker().isCustomTheme(themeName)) {
            continue;
        }

        const ColorThemeWorker::Theme originalTheme = ThemeWorker().getTheme(themeName);
        ColorThemeWorker::Theme updatedTheme;

        for (auto it = renames.begin(); it != renames.end(); ++it) {
            if (originalTheme.contains(it.key())) {
                updatedTheme.insert(it.value(), originalTheme.value(it.key()));
            }
        }

        const QColor bg = originalTheme.value("gui.background", QColor(Qt::white));
        const QStringList renameValues = renames.values();
        for (const auto &option : options) {
            if (renameValues.contains(option)) {
                continue;
            }
            if (originalTheme.contains(option) && !forceDefaultKeys.contains(option)) {
                updatedTheme.insert(option, originalTheme.value(option));
            } else {
                const QColor color =
                        bg.lightness() > 128 ? lightTheme.value(option) : darkTheme.value(option);
                updatedTheme.insert(option, color);
            }
        }

        ThemeWorker().save(updatedTheme, themeName);
        if (Config()->getColorTheme() == themeName) {
            Config()->setColorTheme(themeName);
        }
    }
}

void importOldSettings()
{
    // QSettings
    QSettings::setDefaultFormat(QSettings::IniFormat);
    const QSettings r2CutterSettings(QSettings::IniFormat, QSettings::Scope::UserScope, preRizinOrg,
                                     preRizinApp);
    QSettings newSettings;
    for (const auto &key : r2CutterSettings.allKeys()) {
        newSettings.setValue(key, r2CutterSettings.value(key));
    }

    // Color Themes
    char *szThemes = rz_str_home(".local/share/radare2/cons");
    const QString r2ThemesPath = szThemes;
    rz_mem_free(szThemes);
    const QDir r2ThemesDir(r2ThemesPath);
    if (QFileInfo(r2ThemesPath).isDir()) {
        const QDir rzThemesDir(ThemeWorker().getCustomThemesPath());
        if (!rzThemesDir.exists()) {
            QDir().mkpath(rzThemesDir.absolutePath());
        }
        for (const auto &f : r2ThemesDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            auto dst = rzThemesDir.absoluteFilePath(f.fileName());
            if (QDir(dst).exists()) {
                qInfo() << "Theme" << dst << "already exists. Not overwriting with"
                        << f.absoluteFilePath();
                continue;
            }
            qInfo() << "Copying Theme" << f.absoluteFilePath() << "to" << dst;
            QFile::copy(f.absoluteFilePath(), dst);
        }
    }
}

} // End Anonymous namespace

void Cutter::initializeSettings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    const int settingsVersion = settings.value(CUTTER_SETTINGS_VERSION_KEY, 0).toInt();
    if (settingsVersion == 0) {
        migrateSettingsPre18(settings);
    }

    if (settings.allKeys().length() > 0) {
        if (settingsVersion > CUTTER_SETTINGS_VERSION_CURRENT) {
            qWarning() << "Settings have a higher version than current! Skipping migration.";
        } else if (settingsVersion >= 0) {
            for (int v = settingsVersion + 1; v <= CUTTER_SETTINGS_VERSION_CURRENT; v++) {
                qInfo() << "Migrating Settings to Version" << v;
                switch (v) {
                case 1:
                    migrateSettingsTo1(settings);
                    break;
                case 2:
                    migrateSettingsTo2(settings);
                    break;
                case 3:
                    migrateSettingsTo3(settings);
                    break;
                case 4:
                    migrateSettingsTo4(settings);
                    break;
                case 5:
                    migrateSettingsTo5(settings);
                    break;
                case 6:
                    migrateSettingsTo6(settings);
                    break;
                case 7:
                    migrateSettingsTo7(settings);
                    break;
                default:
                    break;
                }
            }
        }
    }
    settings.setValue(CUTTER_SETTINGS_VERSION_KEY, CUTTER_SETTINGS_VERSION_CURRENT);
}

void Cutter::migrateThemes()
{
    QSettings settings;
    const int themeVersion = settings.value(THEME_VERSION_KEY, 0).toInt();
    if (themeVersion >= THEME_VERSION_CURRENT) {
        qWarning() << "Themes have a higher version than current! Skipping migration.";
        return;
    }
    for (int v = themeVersion + 1; v <= THEME_VERSION_CURRENT; v++) {
        qInfo() << "Migrating Themes to Version" << v;
        switch (v) {
        case 1:
            removeObsoleteOptionsFromCustomThemes();
            break;
        case 2:
            syncCustomThemes();
            break;
        default:
            break;
        }
    }

    settings.setValue(THEME_VERSION_KEY, THEME_VERSION_CURRENT);
}

bool Cutter::shouldOfferSettingImport()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    const QSettings settings;

    if (settings.contains("firstExecution")) {
        return false;
    }
    const QSettings r2CutterSettings(QSettings::IniFormat, QSettings::Scope::UserScope, preRizinOrg,
                                     preRizinApp);
    const QString f = r2CutterSettings.fileName();
    if (r2CutterSettings.value("firstExecution", true) != QVariant(false)) {
        return false; // no Cutter <= 1.12 settings to import
    }
    const int version = r2CutterSettings.value("version", -1).toInt();
    if (version < 1 || version > lastR2CutterSettingVersion) {
        return false; // version too new maybe it's from r2Cutter fork instead of pre-rizin Cutter.
    }
    return true;
}

void Cutter::showSettingImportDialog(int &argc, char **argv)
{
    // Creating temporary QApplication because this happens before anything else in Cutter is
    // initialized
    const QApplication temporaryApp(argc, argv);
    const QSettings r2CutterSettings(QSettings::IniFormat, QSettings::Scope::UserScope, preRizinOrg,
                                     preRizinApp);
    const QString oldFile = r2CutterSettings.fileName();
    // Can't use message translations because settings have not been imported
    auto result =
            QMessageBox::question(nullptr, "Setting import",
                                  QString("Do you want to import settings from %1?").arg(oldFile));
    if (result == QMessageBox::Yes) {
        importOldSettings();
    }
}
