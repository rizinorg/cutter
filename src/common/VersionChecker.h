#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QObject>
#include <functional>

class QNetworkReply;
class QNetworkAccessManager;
class VersionChecker : public QObject
{
    Q_OBJECT
public:
    explicit VersionChecker(QObject *parent = nullptr);

    void checkCurrentVersion(time_t timeoutMs,
                             std::function<void (void)> timeoutCallback = std::function<void(void)>());

signals:
    void checkComplete(const QString& verson, const QString& errorMsg);


private slots:
    void serveVersionCheckReply(QNetworkReply *reply);

private:
    QNetworkAccessManager* nm;
    bool pending;

};

#endif // VERSIONCHECKER_H
