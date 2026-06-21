#ifndef ASYNCTASKDIALOG_H
#define ASYNCTASKDIALOG_H

#include "common/AsyncTask.h"

#include <QDialog>
#include <QTimer>

#include <memory>

namespace Ui {
class AsyncTaskDialog;
}

/**
 * @brief Dialog for displaying progress, logs, and elapsed time @ref AsyncTask
 */
class AsyncTaskDialog : public QDialog
{
    Q_OBJECT

public:
    AsyncTaskDialog(const AsyncTask::Ptr &task, QWidget *parent = nullptr);
    ~AsyncTaskDialog();

    void setInterruptOnClose(bool v) { interruptOnClose = v; }
    bool getInterruptOnClose() const { return interruptOnClose; }

public slots:
    void reject() override;

private slots:
    void updateLog(const QString &log);
    void updateProgressTimer();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::AsyncTaskDialog> ui;
    AsyncTask::Ptr task;
    QTimer timer;

    bool interruptOnClose = false;
};

#endif // ASYNCTASKDIALOG_H
