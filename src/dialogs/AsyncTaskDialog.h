
#ifndef ASYNCTASKDIALOG_H
#define ASYNCTASKDIALOG_H

#include <memory>

#include <QDialog>
#include <QTimer>

namespace Ui {
class AsyncTaskDialog;
}

class AsyncTask;

class AsyncTaskDialog : public QDialog
{
    Q_OBJECT

public:
    AsyncTaskDialog(AsyncTask *task, QWidget *parent = nullptr);
    ~AsyncTaskDialog();

private slots:
    void updateLog();
    void updateProgressTimer();

private:
    std::unique_ptr<Ui::AsyncTaskDialog> ui;
    AsyncTask *task;
    QTimer timer;
};

#endif //ASYNCTASKDIALOG_H
