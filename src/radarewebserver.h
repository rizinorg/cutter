#ifndef RADARE_WEBSERVER_H
#define RADARE_WEBSERVER_H

#include <QThread>
#include <QMutex>

class QRCore;

class RadareWebServer
{
public:

    explicit RadareWebServer(QRCore *core);
    ~RadareWebServer();

    void start();
    void stop();

    bool isStarted() const;

private:
    QRCore *core;
    bool   started;
};

#endif // RADARE_WEBSERVER_H
