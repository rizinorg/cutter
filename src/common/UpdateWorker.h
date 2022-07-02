#ifndef UPDATEWORKER_H
#define UPDATEWORKER_H

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
#    define CUTTER_UPDATE_WORKER_AVAILABLE 1
#else
#    define CUTTER_UPDATE_WORKER_AVAILABLE 0
#endif

#if CUTTER_UPDATE_WORKER_AVAILABLE
#    include <QDir>
#    include <QTimer>
#    include <QObject>
#    include <QtNetwork/QNetworkAccessManager>

#    include <QVersionNumber>
#endif

#if CUTTER_UPDATE_WORKER_AVAILABLE
class QNetworkReply;

/**
 * @class UpdateWorker
 * @brief The UpdateWorker class is a class providing API to check for current Cutter version.
 */

class UpdateWorker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateWorker(QObject *parent = nullptr);

    /**
     * @fn void UpdateWorker::checkCurrentVersion(time_t timeoutMs)
     *
     * Sends request to determine current version of Cutter.
     * If there is no response in @a timeoutMs milliseconds, emits
     * @fn UpdateWorker::checkComplete(const QString& currVerson, const QString& errorMsg)
     * with timeout error message.
     *
     *
     * @sa checkComplete(const QString& verson, const QString& errorMsg)
     */

    void checkCurrentVersion(time_t timeoutMs);

    /**
     * @fn void UpdateWorker::showUpdateDialog()
     *
     * Shows dialog that allows user to download latest version of Cutter from website.
     * This dialog also has "Don't check for updates" button which disables on-start update
     * checks if @a showDontCheckForUpdatesButton is true.
     */
    void showUpdateDialog(bool showDontCheckForUpdatesButton);

    /**
     * @return the version of this Cutter binary, derived from CUTTER_VERSION_MAJOR,
     * CUTTER_VERSION_MINOR and CUTTER_VERSION_PATCH.
     */
    static QVersionNumber currentVersionNumber();

signals:
    /**
     * @fn UpdateWorker::checkComplete(const QString& verson, const QString& errorMsg)
     *
     * The signal is emitted when check has been done with an empty @a errorMsg string.
     * In case of an error @a currVerson is null and @a errorMsg contains description
     * of error.
     */
    void checkComplete(const QVersionNumber &currVerson, const QString &errorMsg);

private slots:
    void serveVersionCheckReply();

private:
    QNetworkAccessManager nm;
    QVersionNumber latestVersion;
    QTimer t;
    bool pending;
    QNetworkReply *checkReply;
};

#endif // CUTTER_UPDATE_WORKER_AVAILABLE
#endif // UPDATEWORKER_H
