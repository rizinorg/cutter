#ifndef ANALTHREAD_H
#define ANALTHREAD_H

#include <QThread>

class CutterCore;

class AnalThread : public QThread
{
    Q_OBJECT
public:
    explicit AnalThread(QWidget *parent = 0);
    ~AnalThread();

    void start(CutterCore *core, int level, QList<QString> advanced);

protected:
    void run();

    using QThread::start;

private:
    CutterCore *core;
    int level;
    QList<QString> advanced;
};

#endif // ANALTHREAD_H
