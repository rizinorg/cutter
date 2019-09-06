
#include "CutterApplication.h"
#include "core/MainWindow.h"
#include "common/UpdateWorker.h"
#include "common/ColorThemeWorker.h"
#include "CutterConfig.h"
#include "common/CrashHandler.h"

#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief Migrate Settings used before Cutter 1.8
 *
 * @return whether any settings have been migrated
 */
static bool migrateSettingsPre18(QSettings &newSettings)
{
    if(newSettings.value("settings_migrated", false).toBool()) {
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

#define CUTTER_SETTINGS_VERSION_CURRENT 2
#define CUTTER_SETTINGS_VERSION_KEY     "version"

/*
 * How Settings migrations work:
 *
 * Every time settings are changed in a way that needs migration,
 * CUTTER_SETTINGS_VERSION_CURRENT is raised by 1 and a function migrateSettingsToX
 * is implemented and added to initializeSettings().
 * This function takes care of migrating from EXACTLY version X-1 to X.
 */

static void migrateSettingsTo1(QSettings &settings) {
    settings.remove("settings_migrated"); // now handled by version
    settings.remove("updated_custom_themes"); // now handled by theme_version
}

static void migrateSettingsTo2(QSettings &settings) {
    QStringList docks = settings.value("docks").toStringList(); // get current list of docks
    // replace occurences of "PseudocodeWidget" with "DecompilerWidget"
    settings.setValue("docks", docks.replaceInStrings("PseudocodeWidget", "DecompilerWidget"));
}

static void initializeSettings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    int settingsVersion = settings.value(CUTTER_SETTINGS_VERSION_KEY, 0).toInt();
    if(settingsVersion == 0) {
        migrateSettingsPre18(settings);
    }

    if(settings.allKeys().length() > 0) {
        if (settingsVersion > CUTTER_SETTINGS_VERSION_CURRENT) {
            qWarning() << "Settings have a higher version than current! Skipping migration.";
        } else if(settingsVersion >= 0) {
            for (int v = settingsVersion + 1; v <= CUTTER_SETTINGS_VERSION_CURRENT; v++) {
                qInfo() << "Migrating Settings to Version" << v;
                switch (v) {
                case 1:
                    migrateSettingsTo1(settings); break;
                case 2:
                    migrateSettingsTo2(settings);
                default:
                    break;
                }
            }
        }
    }
    settings.setValue(CUTTER_SETTINGS_VERSION_KEY, CUTTER_SETTINGS_VERSION_CURRENT);
}


#define THEME_VERSION_CURRENT   1
#define THEME_VERSION_KEY       "theme_version"

static void removeObsoleteOptionsFromCustomThemes() {
    const QStringList options = Core()->cmdj("ecj").object().keys()
        << ColorThemeWorker::cutterSpecificOptions;
    for (auto theme : Core()->cmdList("eco*")) {
        theme = theme.trimmed();
        if (!ThemeWorker().isCustomTheme(theme)) {
            continue;
        }
        QJsonObject updatedTheme;
        auto sch = ThemeWorker().getTheme(theme).object();
        for (const auto& key : sch.keys()) {
            if (options.contains(key)) {
                updatedTheme.insert(key, sch[key]);
            }
        }
        ThemeWorker().save(QJsonDocument(updatedTheme), theme);
    }
}

static void migrateThemes()
{
    QSettings settings;
    int themeVersion = settings.value(THEME_VERSION_KEY, 0).toInt();
    if (themeVersion != THEME_VERSION_CURRENT) {
        removeObsoleteOptionsFromCustomThemes();
        settings.setValue(THEME_VERSION_KEY, THEME_VERSION_CURRENT);
    }
}


int main(int argc, char *argv[])
{
    if (argc >= 3 && QString::fromLocal8Bit(argv[1]) == "--start-crash-handler") {
        QApplication app(argc, argv);
        QString dumpLocation = QString::fromLocal8Bit(argv[2]);
        showCrashDialog(dumpLocation);
        return 0;
    }

    initCrashHandler();

    qRegisterMetaType<QList<StringDescription>>();
    qRegisterMetaType<QList<FunctionDescription>>();

    QCoreApplication::setOrganizationName("RadareOrg");
    QCoreApplication::setApplicationName("Cutter");

    initializeSettings();

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // needed for QtWebEngine inside Plugins

    CutterApplication a(argc, argv);

    migrateThemes();

    if (Config()->getAutoUpdateEnabled()) {
        UpdateWorker *updateWorker = new UpdateWorker;
        QObject::connect(updateWorker, &UpdateWorker::checkComplete,
                         [=](const QVersionNumber &version, const QString & error) {
            if (error.isEmpty() && version > UpdateWorker::currentVersionNumber()) {
                updateWorker->showUpdateDialog(true);
            }
            updateWorker->deleteLater();
        });
        updateWorker->checkCurrentVersion(7000);
    }

    int ret = a.exec();

    return ret;
}
