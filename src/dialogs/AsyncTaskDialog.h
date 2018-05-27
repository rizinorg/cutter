
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

public slots:
    void reject() override;

private slots:
    void updateLog(const QString &log);
    void updateProgressTimer();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::unique_ptr<Ui::AsyncTaskDialog> ui;
    AsyncTask *task;
    QTimer timer;

    bool interruptOnClose;
};

#endif //ASYNCTASKDIALOG_H
