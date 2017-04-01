#include "webserverthread.h"
#include "qrcore.h"
#include <cassert>

WebServerThread::WebServerThread(QRCore *core, QObject *parent) :
    QThread(parent),
    core(core),
    started(false)
{
    // MEOW
}

WebServerThread::~WebServerThread()
{
    if (isRunning()) {
        quit();
        wait();
    }
}

void WebServerThread::startServer()
{
    assert(nullptr != core);

    if (!isRunning() && !started) {
        QThread::start();
    }
}

void WebServerThread::stopServer()
{
    assert(nullptr != core);

    if (!isRunning() && started)
    {
        QThread::start();
    }
}

bool WebServerThread::isStarted() const
{
    QMutexLocker locker(&mutex);
    return started;
}

void WebServerThread::run() {
    QMutexLocker locker(&mutex);

    if (core == nullptr)
        return;
    //eprintf ("Starting webserver!");

    toggleWebServer();
}

void WebServerThread::toggleWebServer()
{
    // access already locked

    // see libr/core/rtr.c
    // "=h", " port", "listen for http connections (r2 -qc=H /bin/ls)",
    // "=h-", "", "stop background webserver",
    // "=h*", "", "restart current webserver",
    // "=h&", " port", "start http server in background)",

    if (started) {
        // after this the only reaction to this commands is:
        // sandbox: connect disabled
        // and the webserver is still running
        // TODO: find out why
        core->cmd("=h-");
    } else {
        core->cmd("=h&");
    }

    // cmd has no usefull return value for this commands, so just toogle the state
    started = !started;
}
