#ifndef ANALTHREAD_H
#define ANALTHREAD_H

#include <QThread>

class CutterCore;
class MainWindow;
class OptionsDialog;

class AnalThread : public QThread
{
    Q_OBJECT
public:
    explicit AnalThread(OptionsDialog *parent = 0);
    ~AnalThread();

    void start(MainWindow *main, int level, QList<QString> advanced);

protected:
    void run();

    using QThread::start;

signals:
    void updateProgress(QString str);

private:
    int level;
    QList<QString> advanced;
    MainWindow *main;
    CutterCore *core;
};

#endif // ANALTHREAD_H
