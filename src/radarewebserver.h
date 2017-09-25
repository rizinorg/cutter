#ifndef RADARE_WEBSERVER_H
#define RADARE_WEBSERVER_H

#include <QThread>
#include <QMutex>

class CutterCore;

class RadareWebServer
{
public:

    explicit RadareWebServer(CutterCore *core);
    ~RadareWebServer();

    void start();
    void stop();

    bool isStarted() const;

private:
    CutterCore *core;
    bool   started;
};

#endif // RADARE_WEBSERVER_H
