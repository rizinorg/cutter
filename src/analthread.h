#ifndef ANALTHREAD_H
#define ANALTHREAD_H

#include <QThread>

class IaitoRCore;

class AnalThread : public QThread
{
    Q_OBJECT
public:
    explicit AnalThread(QWidget *parent = 0);
    ~AnalThread();

    void start(IaitoRCore *core, int level);

protected:
    void run();

    using QThread::start;

private:
    IaitoRCore *core;
    int level;
};

#endif // ANALTHREAD_H
