#include "UpdateWorker.h"

#if CUTTER_UPDATE_WORKER_AVAILABLE
#    include <QUrl>
#    include <QFile>
#    include <QTimer>
#    include <QEventLoop>
#    include <QDataStream>
#    include <QJsonObject>
#    include <QApplication>
#    include <QJsonDocument>
#    include <QDesktopServices>
#    include <QtNetwork/QNetworkReply>
#    include <QtNetwork/QNetworkRequest>
#    include <QStandardPaths>

#    include <QProgressDialog>
#    include <QPushButton>
#    include <QFileDialog>
#    include <QMessageBox>
#    include "common/Configuration.h"
#    include "CutterConfig.h"
#endif

#if CUTTER_UPDATE_WORKER_AVAILABLE
UpdateWorker::UpdateWorker(QObject *parent) : QObject(parent), pending(false)
{
    connect(&t, &QTimer::timeout, this, [this]() {
        if (pending) {
            disconnect(checkReply, nullptr, this, nullptr);
            checkReply->close();
            checkReply->deleteLater();
            emit checkComplete(QVersionNumber(),
                               tr("Time limit exceeded during version check. Please check your "
                                  "internet connection and try again."));
        }
    });
}

void UpdateWorker::checkCurrentVersion(time_t timeoutMs)
{
    QUrl url("https://api.github.com/repos/rizinorg/cutter/releases/latest");
    QNetworkRequest request;
    request.setUrl(url);

    t.setInterval(timeoutMs);
    t.setSingleShot(true);
    t.start();

    checkReply = nm.get(request);
    connect(checkReply, &QNetworkReply::finished, this, &UpdateWorker::serveVersionCheckReply);
    pending = true;
}

void UpdateWorker::showUpdateDialog(bool showDontCheckForUpdatesButton)
{
    QMessageBox mb;
    mb.setWindowTitle(tr("Version control"));
    mb.setText(tr("There is an update available for Cutter.<br/>") + "<b>" + tr("Current version:")
               + "</b> " CUTTER_VERSION_FULL "<br/>" + "<b>" + tr("Latest version:") + "</b> "
               + latestVersion.toString() + "<br/><br/>"
               + tr("To update, please check the link:<br/>")
               + QString("<a href=\"https://github.com/rizinorg/cutter/releases/tag/v%1\">"
                         "https://github.com/rizinorg/cutter/releases/tag/v%1</a><br/>")
                         .arg(latestVersion.toString()));
    if (showDontCheckForUpdatesButton) {
        mb.setStandardButtons(QMessageBox::Reset | QMessageBox::Ok);
        mb.button(QMessageBox::Reset)->setText(tr("Don't check for updates automatically"));
    } else {
        mb.setStandardButtons(QMessageBox::Ok);
    }
    mb.setDefaultButton(QMessageBox::Ok);
    int ret = mb.exec();
    if (ret == QMessageBox::Reset) {
        Config()->setAutoUpdateEnabled(false);
    }
}

void UpdateWorker::serveVersionCheckReply()
{
    pending = false;
    QString versionReplyStr = "";
    QString errStr = "";
    if (checkReply->error()) {
        errStr = checkReply->errorString();
    } else {
        versionReplyStr = QJsonDocument::fromJson(checkReply->readAll())
                                  .object()
                                  .value("tag_name")
                                  .toString();
        versionReplyStr.remove('v');
    }
    QVersionNumber versionReply = QVersionNumber::fromString(versionReplyStr);
    if (!versionReply.isNull()) {
        latestVersion = versionReply;
    }
    checkReply->close();
    checkReply->deleteLater();
    emit checkComplete(versionReply, errStr);
}

QVersionNumber UpdateWorker::currentVersionNumber()
{
    return QVersionNumber(CUTTER_VERSION_MAJOR, CUTTER_VERSION_MINOR, CUTTER_VERSION_PATCH);
}
#endif // CUTTER_UPDATE_WORKER_AVAILABLE
