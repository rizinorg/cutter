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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(AnalysisTask)                                                           \
        AnalysisTask(const AnalysisTask &w) = delete;                                              \
        AnalysisTask &operator=(const AnalysisTask &w) = delete;

#    define Q_DISABLE_MOVE(AnalysisTask)                                                           \
        AnalysisTask(AnalysisTask &&w) = delete;                                                   \
        AnalysisTask &operator=(AnalysisTask &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(AnalysisTask)                                                      \
        Q_DISABLE_COPY(AnalysisTask)                                                               \
        Q_DISABLE_MOVE(AnalysisTask)
#endif

    Q_DISABLE_COPY_MOVE(AnalysisTask)

public:
    explicit AnalysisTask();
    ~AnalysisTask() override;

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
