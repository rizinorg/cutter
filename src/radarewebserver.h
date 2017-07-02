#ifndef RADARE_WEBSERVER_H
#define RADARE_WEBSERVER_H

#include <QThread>
#include <QMutex>

class IaitoRCore;

class RadareWebServer
{
public:

    explicit RadareWebServer(IaitoRCore *core);
    ~RadareWebServer();

    void start();
    void stop();

    bool isStarted() const;

private:
    IaitoRCore *core;
    bool   started;
};

#endif // RADARE_WEBSERVER_H
