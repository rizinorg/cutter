#ifndef WEBSERVERTHREAD_H
#define WEBSERVERTHREAD_H

#include <QThread>
#include <QMutex>

class QRCore;

class WebServerThread : public QThread
{
    Q_OBJECT
public:

    explicit WebServerThread(QRCore *core, QObject *parent = 0);
    ~WebServerThread();

    void startServer();
    void stopServer();

    bool isStarted() const;

private:
    void run();
    using QThread::start;

    void toggleWebServer();

    mutable QMutex mutex;
    QRCore *core;
    bool   started;
};

#endif // WEBSERVERTHREAD_H
