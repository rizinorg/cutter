#include "webserverthread.h"

WebServerThread::WebServerThread(QObject *parent) :
    QThread(parent)
{
    // MEOW
}

void WebServerThread::run() {
    if (core == NULL)
        return;
    //eprintf ("Starting webserver!");
    core->cmd ("=h");
}
