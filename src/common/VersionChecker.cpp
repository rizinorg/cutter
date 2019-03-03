#include "VersionChecker.h"

#include <QUrl>
#include <QTimer>
#include <QEventLoop>
#include <QJsonObject>
#include <QApplication>
#include <QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

VersionChecker::VersionChecker(QObject *parent) :
    QObject(parent), pending(false)
{
    connect(&nm, &QNetworkAccessManager::finished, this, &VersionChecker::serveVersionCheckReply);
    connect(&t, &QTimer::timeout, [this]() {
        if (pending) {
            emit checkComplete("", tr("Time limit exceeded during version check. Please check your "
                                      "internet connection and try again."));
        }
    });
}

void VersionChecker::checkCurrentVersion(time_t timeoutMs)
{
    QUrl url("https://api.github.com/repos/radareorg/cutter/releases/latest");
    QNetworkRequest request;
    request.setUrl(url);

    t.setInterval(timeoutMs);
    t.setSingleShot(true);
    t.start();

    nm.get(request);
    pending = true;
}

void VersionChecker::serveVersionCheckReply(QNetworkReply *reply)
{
    pending = false;
    QString currVersion = "";
    QString errStr = "";
    if (reply->error()) {
        errStr = reply->errorString();
    } else {
        currVersion = QJsonDocument::fromJson(reply->readAll()).object().value("tag_name").toString();
        currVersion.remove('v');
    }
    delete reply;
    emit checkComplete(currVersion, errStr);
}
