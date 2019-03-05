
#include "CutterApplication.h"
#include "core/MainWindow.h"
#include "common/UpdateWorker.h"
#include <QProgressDialog>
#include <CutterConfig.h>
#include <QPushButton>
#include <QFileDialog>

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
        UpdateWorker *updateWorker = new UpdateWorker;
        QObject::connect(updateWorker, &UpdateWorker::checkComplete,
                         [=](const QString & version, const QString & error) {
            if (error == "" && version == CUTTER_VERSION_FULL) {
                QMessageBox mb;
                mb.setWindowTitle(QObject::tr("Version control"));
                mb.setText("<b>" + QObject::tr("Current version:") + "</b> " CUTTER_VERSION_FULL "<br/>"
                           + "<b>" + QObject::tr("Latest version:") + "</b> " + version + "<br/><br/>"
                           + QObject::tr("For update, please check the link:<br/>")
                           + QString("<a href=\"https://github.com/radareorg/cutter/releases/tag/v%1\">"
                                     "https://github.com/radareorg/cutter/releases/tag/v%1</a><br/>").arg(version)
                           + QObject::tr("or click \"Download\" to download latest version of Cutter."));
                mb.setStandardButtons(QMessageBox::Ok |
                                      QMessageBox::No |
                                      QMessageBox::Yes);
                mb.button(QMessageBox::No)->setText(QObject::tr("Don't check for updates"));
                mb.button(QMessageBox::Yes)->setText(QObject::tr("Download"));
                mb.setDefaultButton(QMessageBox::Ok);
                int ret = mb.exec();
                if (ret == QMessageBox::No) {
                    Config()->setAutoUpdateEnabled(false);
                    updateWorker->deleteLater();
                } else if (ret == QMessageBox::Yes) {
                    QProgressDialog progressDial(QObject::tr("Downloading..."),
                                                 QObject::tr("Cancel"),
                                                 0, 100);
                    QObject::connect(updateWorker, &UpdateWorker::downloadProcess,
                                     [&progressDial](size_t curr, size_t total) {
                        progressDial.setValue(100.0f * curr / total);
                    });
                    QObject::connect(&progressDial, &QProgressDialog::canceled,
                                     updateWorker, &UpdateWorker::abortDownload);
                    QObject::connect(updateWorker, &UpdateWorker::downloadFinished,
                                     &progressDial, &QProgressDialog::cancel);
                    QObject::connect(updateWorker, &UpdateWorker::downloadFinished,
                                     [](QString filePath){
                        // I couldnt figure out what to write here
                        // so by now I left it as it is
                        QMessageBox::information(nullptr,
                                                 QObject::tr("Download finished!"),
                                                 filePath);
                    });

                    QDir downloadDir =
                            QFileDialog::getExistingDirectory(nullptr,
                                                              QObject::tr("Choose directory"
                                                                          "for downloading"),
                                                              QStandardPaths::writableLocation(
                                                                  QStandardPaths::HomeLocation));
                    if (downloadDir.path() != ".") {
                        updateWorker->download(downloadDir, version);
                        progressDial.exec();
                    }
                } else {
                    updateWorker->deleteLater();
                    QObject::connect(updateWorker, &UpdateWorker::downloadFinished,
                                     updateWorker, &UpdateWorker::deleteLater);
                }
            }
        });
        updateWorker->checkCurrentVersion(7000);
    }

    int ret = a.exec();

    return ret;
}
