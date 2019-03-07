#include "UpdateWorker.h"

#include <QUrl>
#include <QFile>
#include <QTimer>
#include <QEventLoop>
#include <QDataStream>
#include <QJsonObject>
#include <QApplication>
#include <QJsonDocument>
#include <QDesktopServices>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QProgressDialog>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include "common/Configuration.h"
#include "CutterConfig.h"

UpdateWorker::UpdateWorker(QObject *parent) :
    QObject(parent), latestVersion(""), pending(false)
{
    connect(&t, &QTimer::timeout, [this]() {
        if (pending) {
            disconnect(checkReply, nullptr, this, nullptr);
            checkReply->close();
            checkReply->deleteLater();
            emit checkComplete("", tr("Time limit exceeded during version check. Please check your "
                                      "internet connection and try again."));
        }
    });
}

void UpdateWorker::checkCurrentVersion(time_t timeoutMs)
{
    QUrl url("https://api.github.com/repos/radareorg/cutter/releases/latest");
    QNetworkRequest request;
    request.setUrl(url);

    t.setInterval(timeoutMs);
    t.setSingleShot(true);
    t.start();

    checkReply = nm.get(request);
    connect(checkReply, &QNetworkReply::finished,
            this, &UpdateWorker::serveVersionCheckReply);
    pending = true;
}

void UpdateWorker::download(QString filename, QString version)
{
    downloadFile.setFileName(filename);
    downloadFile.open(QIODevice::WriteOnly);

    QNetworkRequest request;
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QUrl url(QString("https://github.com/radareorg/cutter/releases/"
                     "download/v%1/%2").arg(version).arg(getRepositoryFileName()));
    request.setUrl(url);

    downloadReply = nm.get(request);
    connect(downloadReply, &QNetworkReply::downloadProgress,
            this, &UpdateWorker::process);
    connect(downloadReply, &QNetworkReply::finished,
            this, &UpdateWorker::serveDownloadFinish);
}

void UpdateWorker::showUpdateDialog(bool showDontCheckForUpdatesButton)
{
    QMessageBox mb;
    mb.setWindowTitle(tr("Version control"));
    mb.setText(tr("There is an update available for Cutter.<br/>")
               + "<b>" + tr("Current version:") + "</b> " CUTTER_VERSION_FULL "<br/>"
               + "<b>" + tr("Latest version:") + "</b> " + latestVersion + "<br/><br/>"
               + tr("For update, please check the link:<br/>")
               + QString("<a href=\"https://github.com/radareorg/cutter/releases/tag/v%1\">"
                         "https://github.com/radareorg/cutter/releases/tag/v%1</a><br/>").arg(latestVersion)
               + tr("or click \"Download\" to download latest version of Cutter."));
    if (showDontCheckForUpdatesButton) {
        mb.setStandardButtons(QMessageBox::Save | QMessageBox::No | QMessageBox::Ok);
        mb.button(QMessageBox::No)->setText(tr("Don't check for updates"));
    } else {
        mb.setStandardButtons(QMessageBox::Save | QMessageBox::Ok);
    }
    mb.button(QMessageBox::Save)->setText(tr("Download"));
    mb.setDefaultButton(QMessageBox::Ok);
    int ret = mb.exec();
    if (ret == QMessageBox::No) {
        Config()->setAutoUpdateEnabled(false);
    } else if (ret == QMessageBox::Save) {
        QString fullFileName =
                QFileDialog::getSaveFileName(nullptr,
                                             tr("Choose directory for downloading"),
                                             QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                                             QDir::separator() + getRepositoryFileName(),
                                             QString("%1 (*.%1)").arg(getRepositeryExt()));
        if (fullFileName != "") {
            QProgressDialog progressDial(tr("Downloading..."),
                                         tr("Cancel"),
                                         0, 100);
            connect(this, &UpdateWorker::downloadProcess,
                    [&progressDial](size_t curr, size_t total) {
                progressDial.setValue(100.0f * curr / total);
            });
            connect(&progressDial, &QProgressDialog::canceled,
                    this, &UpdateWorker::abortDownload);
            connect(this, &UpdateWorker::downloadFinished,
                    &progressDial, &QProgressDialog::cancel);
            connect(this, &UpdateWorker::downloadFinished,
                    [](QString filePath){
                QMessageBox info(QMessageBox::Information,
                                 tr("Download finished!"),
                                 tr("Latest version of Cutter was succesfully downloaded!"),
                                 QMessageBox::Yes | QMessageBox::Open | QMessageBox::Ok,
                                 nullptr);
                info.button(QMessageBox::Open)->setText(tr("Open file"));
                info.button(QMessageBox::Yes)->setText(tr("Open download folder"));
                int r = info.exec();
                if (r == QMessageBox::Open) {
                    QDesktopServices::openUrl(filePath);
                } else if (r == QMessageBox::Yes) {
                    auto path = filePath.split(QDir::separator());
                    path.removeLast();
                    QDesktopServices::openUrl(path.join(QDir::separator()));
                }
            });
            download(fullFileName, latestVersion);
            progressDial.exec();
        }
    }
}

void UpdateWorker::abortDownload()
{
    disconnect(downloadReply, &QNetworkReply::finished,
               this, &UpdateWorker::serveDownloadFinish);
    disconnect(downloadReply, &QNetworkReply::downloadProgress,
               this, &UpdateWorker::process);
    downloadReply->close();
    downloadReply->deleteLater();
    downloadFile.remove();
}

void UpdateWorker::serveVersionCheckReply()
{
    pending = false;
    QString versionReply = "";
    QString errStr = "";
    if (checkReply->error()) {
        errStr = checkReply->errorString();
    } else {
        versionReply = QJsonDocument::fromJson(checkReply->readAll()).object().value("tag_name").toString();
        versionReply.remove('v');
    }
    latestVersion = versionReply;
    checkReply->close();
    checkReply->deleteLater();
    emit checkComplete(versionReply, errStr);
}

void UpdateWorker::serveDownloadFinish()
{
    downloadReply->close();
    downloadReply->deleteLater();
    if (downloadReply->error()) {
        emit downloadError(downloadReply->errorString());
    } else {
        emit downloadFinished(downloadFile.fileName());
    }
}

void UpdateWorker::process(size_t bytesReceived, size_t bytesTotal)
{
    downloadFile.write(downloadReply->readAll());
    emit downloadProcess(bytesReceived, bytesTotal);
}

QString UpdateWorker::getRepositeryExt() const
{
#ifdef Q_OS_LINUX
    return "AppImage";
#elif defined (Q_OS_WIN64) || defined (Q_OS_WIN32)
    return "zip";
#elif defined (Q_OS_MACOS)
    return "dmg";
#endif
}

QString UpdateWorker::getRepositoryFileName() const
{
    QString downloadFileName;
#ifdef Q_OS_LINUX
    downloadFileName = "Cutter-v%1-x%2.Linux.AppImage";
#elif defined (Q_OS_WIN64) || defined (Q_OS_WIN32)
    downloadFileName = "Cutter-v%1-x%2.Windows.zip";
#elif defined (Q_OS_MACOS)
    downloadFileName = "Cutter-v%1-x%2.macOS.dmg";
#endif
    downloadFileName = downloadFileName
                       .arg(latestVersion)
                       .arg(QSysInfo::buildAbi().split('-').at(2).contains("64")
                            ? "64"
                            : "32");

    return downloadFileName;
}
