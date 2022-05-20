#include "SettingsUpgrade.h"
#include <QApplication>
#include <QMessageBox>

#include "common/ColorThemeWorker.h"

/**
 * @brief Migrate Settings used before Cutter 1.8
 *
 * @return whether any settings have been migrated
 */
static bool migrateSettingsPre18(QSettings &newSettings)
{
    if (newSettings.value("settings_migrated", false).toBool()) {
        return false;
    }
    QSettings oldSettings(QSettings::NativeFormat, QSettings::Scope::UserScope, "Cutter", "Cutter");
    QStringList allKeys = oldSettings.allKeys();
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

#define CUTTER_SETTINGS_VERSION_CURRENT 6
#define CUTTER_SETTINGS_VERSION_KEY "version"

/*
 * How Settings migrations work:
 *
 * Every time settings are changed in a way that needs migration,
 * CUTTER_SETTINGS_VERSION_CURRENT is raised by 1 and a function migrateSettingsToX
 * is implemented and added to initializeSettings().
 * This function takes care of migrating from EXACTLY version X-1 to X.
 */

static void migrateSettingsTo1(QSettings &settings)
{
    settings.remove("settings_migrated"); // now handled by version
    settings.remove("updated_custom_themes"); // now handled by theme_version
}

static void migrateSettingsTo2(QSettings &settings)
{
    QStringList docks = settings.value("docks").toStringList(); // get current list of docks
    // replace occurences of "PseudocodeWidget" with "DecompilerWidget"
    settings.setValue("docks", docks.replaceInStrings("PseudocodeWidget", "DecompilerWidget"));
}

static void migrateSettingsTo3(QSettings &settings)
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
    QSet<QString> unsyncDocks(unsyncList.begin(), unsyncList.end());
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

static void renameAsmOption(QSettings &settings, const QString &oldName, const QString &newName)
{
    if (settings.contains(oldName)) {
        auto value = settings.value(oldName);
        settings.remove(oldName);
        settings.setValue(newName, value);
    }
}

static void migrateSettingsTo4(QSettings &settings)
{
    renameAsmOption(settings, "asm.var.subonly", "asm.sub.varonly");
    renameAsmOption(settings, "asm.bytespace", "asm.bytes.space");
}

static void migrateSettingsTo5(QSettings &settings)
{
    renameAsmOption(settings, "asm.var.sub", "asm.sub.var");
}

static void migrateSettingsTo6(QSettings &settings)
{
    settings.remove("dir.projects");
}

void Cutter::initializeSettings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    int settingsVersion = settings.value(CUTTER_SETTINGS_VERSION_KEY, 0).toInt();
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
                default:
                    break;
                }
            }
        }
    }
    settings.setValue(CUTTER_SETTINGS_VERSION_KEY, CUTTER_SETTINGS_VERSION_CURRENT);
}

#define THEME_VERSION_CURRENT 1
#define THEME_VERSION_KEY "theme_version"

static void removeObsoleteOptionsFromCustomThemes()
{
    const QStringList options = Core()->getThemeKeys() << ColorThemeWorker::cutterSpecificOptions;
    QStringList themes = Core()->getColorThemes();
    for (const auto &themeName : themes) {
        if (!ThemeWorker().isCustomTheme(themeName)) {
            continue;
        }
        ColorThemeWorker::Theme sch = ThemeWorker().getTheme(themeName);
        ColorThemeWorker::Theme updatedTheme;
        for (auto it = sch.constBegin(); it != sch.constEnd(); ++it) {
            if (options.contains(it.key())) {
                updatedTheme.insert(it.key(), it.value());
            }
        }
        ThemeWorker().save(updatedTheme, themeName);
    }
}

void Cutter::migrateThemes()
{
    QSettings settings;
    int themeVersion = settings.value(THEME_VERSION_KEY, 0).toInt();
    if (themeVersion != THEME_VERSION_CURRENT) {
        removeObsoleteOptionsFromCustomThemes();
        settings.setValue(THEME_VERSION_KEY, THEME_VERSION_CURRENT);
    }
}

static const char PRE_RIZIN_ORG[] = "RadareOrg";
static const char PRE_RIZIN_APP[] = "Cutter";

bool Cutter::shouldOfferSettingImport()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;
    if (settings.contains("firstExecution")) {
        return false;
    }
    QSettings r2CutterSettings(QSettings::IniFormat, QSettings::Scope::UserScope, PRE_RIZIN_ORG,
                               PRE_RIZIN_APP);
    QString f = r2CutterSettings.fileName();
    if (r2CutterSettings.value("firstExecution", true) != QVariant(false)) {
        return false; // no Cutter <= 1.12 settings to import
    }
    int version = r2CutterSettings.value("version", -1).toInt();
    if (version < 1 || version > 6) {
        return false; // version too new maybe it's from r2Cutter fork instead of pre-rizin Cutter.
    }
    return true;
}

static void importOldSettings()
{
    // QSettings
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings r2CutterSettings(QSettings::IniFormat, QSettings::Scope::UserScope, PRE_RIZIN_ORG,
                               PRE_RIZIN_APP);
    QSettings newSettings;
    for (auto key : r2CutterSettings.allKeys()) {
        newSettings.setValue(key, r2CutterSettings.value(key));
    }

    // Color Themes
    char *szThemes = rz_str_home(".local/share/radare2/cons");
    QString r2ThemesPath = szThemes;
    rz_mem_free(szThemes);
    QDir r2ThemesDir(r2ThemesPath);
    if (QFileInfo(r2ThemesPath).isDir()) {
        QDir rzThemesDir(ThemeWorker().getCustomThemesPath());
        if (!rzThemesDir.exists()) {
            QDir().mkpath(rzThemesDir.absolutePath());
        }
        for (auto f : r2ThemesDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
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

void Cutter::showSettingImportDialog(int &argc, char **argv)
{
    // Creating temporary QApplication because this happens before anything else in Cutter is
    // initialized
    QApplication temporaryApp(argc, argv);
    QSettings r2CutterSettings(QSettings::IniFormat, QSettings::Scope::UserScope, PRE_RIZIN_ORG,
                               PRE_RIZIN_APP);
    QString oldFile = r2CutterSettings.fileName();
    // Can't use message translations because settings have not been imported
    auto result =
            QMessageBox::question(nullptr, "Setting import",
                                  QString("Do you want to import settings from %1?").arg(oldFile));
    if (result == QMessageBox::Yes) {
        importOldSettings();
    }
}
