
#include "CutterApplication.h"
#include "core/MainWindow.h"
#include "common/VersionChecker.h"
#include <CutterConfig.h>
#include <QPushButton>

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

    CutterApplication a(argc, argv);

    if (Config()->getAutoUpdateEnabled()) {
        VersionChecker *versionChecker = new VersionChecker;
        QObject::connect(versionChecker, &VersionChecker::checkComplete,
        [](const QString & version, const QString & error) {
            if (error == "" && version != CUTTER_VERSION_FULL) {
                QMessageBox mb;
                mb.setWindowTitle(QObject::tr("Version control"));
                mb.setText("<b>" + QObject::tr("Current version:") + "</b> " CUTTER_VERSION_FULL "<br/>"
                           + "<b>" + QObject::tr("Latest version:") + "</b> " + version + "<br/><br/>"
                           + QObject::tr("For update, please check the link:<br/>")
                           + "<a href=\"https://github.com/radareorg/cutter/releases\">"
                           + "https://github.com/radareorg/cutter/releases</a>");
                mb.setStandardButtons(QMessageBox::Ok | QMessageBox::No);
                mb.button(QMessageBox::No)->setText(QObject::tr("Don't check for updates."));
                mb.setDefaultButton(QMessageBox::Ok);
                int ret = mb.exec();
                if (ret == QMessageBox::No) {
                    Config()->setAutoUpdateEnabled(false);
                }
            }
        });
        QObject::connect(versionChecker, &VersionChecker::checkComplete,
                         versionChecker, &VersionChecker::deleteLater);
        versionChecker->checkCurrentVersion(7000);
    }

    int ret = a.exec();

    return ret;
}
