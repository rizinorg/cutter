#ifndef ANALTHREAD_H
#define ANALTHREAD_H

#include "utils/AsyncTask.h"

class CutterCore;
class MainWindow;
class OptionsDialog;

class AnalTask : public AsyncTask
{
    Q_OBJECT

public:
    explicit AnalTask(OptionsDialog *parent = nullptr);
    ~AnalTask();

    void setSettings(MainWindow *main, int level, QList<QString> advanced);
    void interrupt() override;
    void interruptAndWait();

protected:
    void runTask() override;

signals:
    void openFileFailed();

private:
    int level;
    QList<QString> advanced;
    MainWindow *main;
};

#endif // ANALTHREAD_H
