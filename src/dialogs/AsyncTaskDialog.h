
#ifndef ASYNCTASKDIALOG_H
#define ASYNCTASKDIALOG_H

#include <QDialog>
#include <memory>

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

private:
    std::unique_ptr<Ui::AsyncTaskDialog> ui;
    AsyncTask *task;
};

#endif //ASYNCTASKDIALOG_H
