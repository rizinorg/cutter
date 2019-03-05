#ifndef UPDATEWORKER_H
#define UPDATEWORKER_H

#include <QDir>
#include <QTimer>
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class QNetworkReply;

/*!
    \class VersionChecker
    \brief The VersionChecker class is a class providing API to check for current Cutter version.
*/

class UpdateWorker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateWorker(QObject *parent = nullptr);

    /*!
      \fn void UpdateWorker::checkCurrentVersion(time_t timeoutMs)

      Sends request to determine current version of Cutter.
      If there is no response in \a timeoutMs milliseconds, emits
      \fn UpdateWorker::checkComplete(const QString& currVerson, const QString& errorMsg)
      with timeout error message.


      \sa checkComplete(const QString& verson, const QString& errorMsg)
    */

    void checkCurrentVersion(time_t timeoutMs);

    /*!
      \fn void UpdateWorker::download(QDir downloadPath, QString version)

      \brief Downloads provided \a version of Cutter into \a downloadDir.

      \sa downloadProcess(size_t bytesReceived, size_t bytesTotal)
    */
    void download(QDir downloadDir, QString version);

public slots:
    void abortDownload();

signals:
    /*!
      \fn UpdateWorker::checkComplete(const QString& verson, const QString& errorMsg)

      The signal is emitted when check has been done with an empty \a errorMsg string.
      In case of an error \a currVerson is empty and \a errorMsg contains description
      of error.

    */
    void checkComplete(const QString &currVerson, const QString &errorMsg);

    /*!
      \fn UpdateWorker::downloadProcess(size_t bytesReceived, size_t bytesTotal)

      The signal is emitted each time when some amount of bytes was downloaded.
      May be used as indicator of download progress.

    */
    void downloadProcess(size_t bytesReceived, size_t bytesTotal);

    void downloadFinished(QString filename);


private slots:
    void serveVersionCheckReply();

    void serveDownloadFinish();

    void process(size_t bytesReceived, size_t bytesTotal);

private:
    QNetworkAccessManager nm;
    QTimer t;
    bool pending;
    QFile downloadFile;
    QNetworkReply *downloadReply;
};

#endif // UPDATEWORKER_H
