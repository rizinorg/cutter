#ifndef WEBSERVERTHREAD_H
#define WEBSERVERTHREAD_H

#include <QThread>
#include "qrcore.h"

class WebServerThread : public QThread
{
    Q_OBJECT
public:
    QRCore *core;
    explicit WebServerThread(QObject *parent = 0);

signals:

public slots:
private:
    void run();
};

#endif // WEBSERVERTHREAD_H
