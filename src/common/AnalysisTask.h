#ifndef ANALTHREAD_H
#define ANALTHREAD_H

#include "common/AsyncTask.h"
#include "core/Cutter.h"
#include "common/InitialOptions.h"

class CutterCore;
class MainWindow;
class InitialOptionsDialog;

class AnalysisTask : public AsyncTask
{
    Q_OBJECT

public:
    explicit AnalysisTask();
    ~AnalysisTask();

    QString getTitle() override;

    void setOptions(const InitialOptions &options) { this->options = options; }

    void interrupt() override;

    bool getOpenFileFailed() { return openFailed; }

protected:
    void runTask() override;

signals:
    void openFileFailed();

private:
    InitialOptions options;

    bool openFailed = false;
};

#endif // ANALTHREAD_H
