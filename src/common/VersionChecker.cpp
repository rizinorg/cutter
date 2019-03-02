#include "VersionChecker.h"

#include <QUrl>
#include <QTimer>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

VersionChecker::VersionChecker(QObject *parent) :
    QObject(parent), nm(new QNetworkAccessManager), pending(false) { }

void VersionChecker::checkCurrentVersion(time_t timeoutMs, std::function<void (void)> timeoutCallback)
{
    QUrl url("https://api.github.com/repos/radareorg/cutter/releases/latest");
    QNetworkRequest request;
    request.setUrl(url);

    connect(&t, &QTimer::timeout, [timeoutCallback, this]() {
        if (pending) {
            disconnect(nm, &QNetworkAccessManager::finished, this, &VersionChecker::serveVersionCheckReply);
            if (timeoutCallback) {
                timeoutCallback();
            }
        }
    });
    t.setInterval(timeoutMs);
    t.setSingleShot(true);
    t.start();

    connect(nm, &QNetworkAccessManager::finished, this, &VersionChecker::serveVersionCheckReply);

    nm->get(request);
    pending = true;
}

void VersionChecker::serveVersionCheckReply(QNetworkReply* reply)
{
    pending = false;
    QString currVersion = "";
    if (reply->error()) {
        emit checkComplete("", reply->errorString());
    } else {
        currVersion = QJsonDocument::fromJson(reply->readAll()).object().value("tag_name").toString();
        currVersion.remove('v');
        emit checkComplete(currVersion, "");
    }
    delete reply;
}
