
#include "CutterApplication.h"
#include "core/MainWindow.h"
#include "common/UpdateWorker.h"
#include "common/ColorThemeWorker.h"
#include "CutterConfig.h"
#include "common/CrashHandler.h"

#include <QJsonObject>

/**
 * @brief Migrate Settings used before Cutter 1.8
 */
static void migrateSettings(QSettings &newSettings)
{
    QSettings oldSettings(QSettings::NativeFormat, QSettings::Scope::UserScope, "Cutter", "Cutter");
    for (const QString &key : oldSettings.allKeys()) {
        newSettings.setValue(key, oldSettings.value(key));
    }
    oldSettings.clear();
    QFile settingsFile(oldSettings.fileName());
    settingsFile.remove();
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
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings settings;
    if (!settings.value("settings_migrated", false).toBool()) {
        qInfo() << "Settings have not been migrated before, trying to migrate from pre-1.8 if possible.";
        migrateSettings(settings);
        settings.setValue("settings_migrated", true);
    }

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // needed for QtWebEngine inside Plugins

    CutterApplication a(argc, argv);

    // Removes obsolete color options (highlight and highlightWord) from custom theme files
    if (!settings.value("updated_custom_themes", false).toBool()) {
        const QStringList options = Core()->cmdj("ecj").object().keys()
                                    << ColorThemeWorker::cutterSpecificOptions;
        for (auto theme : Core()->cmdList("eco*")) {
            theme = theme.trimmed();
            if (!ThemeWorker().isCustomTheme(theme)) {
                continue;
            }
            QJsonObject updatedTheme;
            auto sch = ThemeWorker().getTheme(theme);
            for (auto key : sch.object().keys()) {
                if (options.contains(key)) {
                    updatedTheme.insert(key, sch[key]);
                }
            }
            ThemeWorker().save(QJsonDocument(updatedTheme), theme);
        }
        settings.setValue("updated_custom_themes", true);
    }

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
