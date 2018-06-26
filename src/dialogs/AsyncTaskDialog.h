
#ifndef ASYNCTASKDIALOG_H
#define ASYNCTASKDIALOG_H

#include <memory>

#include <QDialog>
#include <QTimer>

#include "utils/AsyncTask.h"

namespace Ui {
class AsyncTaskDialog;
}

class AsyncTaskDialog : public QDialog
{
    Q_OBJECT

public:
    AsyncTaskDialog(AsyncTask::Ptr task, QWidget *parent = nullptr);
    ~AsyncTaskDialog();

    void setInterruptOnClose(bool v)        { interruptOnClose = v; }
    bool getInterruptOnClose()              { return interruptOnClose; }

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

#endif //ASYNCTASKDIALOG_H
