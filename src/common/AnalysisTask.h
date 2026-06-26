#ifndef ANALYSISTASK_H
#define ANALYSISTASK_H

#include "common/AsyncTask.h"
#include "common/InitialOptions.h"
// #include "core/Cutter.h"

class CutterCore;
class MainWindow;
class InitialOptionsDialog;

/**
 * @brief Background task for initial binary analysis and file loading
 */
class AnalysisTask : public AsyncTask
{
    Q_OBJECT

public:
    explicit AnalysisTask();
    ~AnalysisTask();

    QString getTitle() const override;

    void setOptions(const InitialOptions &options) { this->options = options; }

    void interrupt() override;

    bool getOpenFileFailed() const { return openFailed; }

protected:
    void runTask() override;

signals:
    void openFileFailed();

private:
    InitialOptions options;

    bool openFailed = false;
};

#endif // ANALYSISTASK_H
