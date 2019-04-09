
#include "CutterApplication.h"
#include "core/MainWindow.h"
#include "common/UpdateWorker.h"
#include "CutterConfig.h"
#include "common/CrashHandler.h"

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
