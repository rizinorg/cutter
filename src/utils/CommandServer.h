#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include <QObject>

class CommandServer : public QObject
{
    Q_OBJECT
public:
    explicit CommandServer(QObject *parent = nullptr);
    ~CommandServer();
    void stop();

signals:
    void finished();
    void error(QString err);

public slots:
    void process();

private:
    bool isRunning = true;
    bool startCommandServer();
};

#endif // COMMANDSERVER_H
