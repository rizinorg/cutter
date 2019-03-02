#ifndef VERSIONCHECKER_H
#define VERSIONCHECKER_H

#include <QTimer>
#include <QObject>

class QNetworkReply;
class QNetworkAccessManager;

/*!
    \class VersionChecker
    \brief The VersionChecker class is a class providing API to check for current Cutter version.
*/

class VersionChecker : public QObject
{
    Q_OBJECT
public:
    explicit VersionChecker(QObject *parent = nullptr);

    /*!
      \fn void VersionChecker::checkCurrentVersion(time_t timeoutMs)

      Sends request to determine current version of Cutter.
      If there are no respond in \a timeoutMs milliseconds, emits
      \fn VersionChecker::checkComplete(const QString& currVerson, const QString& errorMsg)
      with timeout error message.


      \sa checkComplete(const QString& verson, const QString& errorMsg)
    */

    void checkCurrentVersion(time_t timeoutMs);

signals:
    /*!
      \fn VersionChecker::checkComplete(const QString& verson, const QString& errorMsg)

      The signal is emited when check has been done with empty \a errorMsg string.
      In case of error \a currVerson is empty and \a errorMsg contains description
      of error.

    */
    void checkComplete(const QString& currVerson, const QString& errorMsg);

private slots:
    void serveVersionCheckReply(QNetworkReply *reply);

private:
    QNetworkAccessManager* nm;
    QTimer t;
    bool pending;
};

#endif // VERSIONCHECKER_H
