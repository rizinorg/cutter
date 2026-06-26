
#include "AsyncTaskDialog.h"

#include "common/AsyncTask.h"
#include "ui_AsyncTaskDialog.h"

AsyncTaskDialog::AsyncTaskDialog(const AsyncTask::Ptr &task, QWidget *parent)
    : QDialog(parent), ui(new Ui::AsyncTaskDialog), task(task)
{
    ui->setupUi(this);

    const QString title = task->getTitle();
    if (!title.isNull()) {
        setWindowTitle(title);
    }

    connect(task.get(), &AsyncTask::logChanged, this, &AsyncTaskDialog::updateLog);
    connect(task.get(), &AsyncTask::finished, this, [this]() { close(); });

    updateLog(task->getLog());

    connect(&timer, &QTimer::timeout, this, &AsyncTaskDialog::updateProgressTimer);
    timer.setInterval(1000);
    timer.setSingleShot(false);
    timer.start();

    updateProgressTimer();
}

AsyncTaskDialog::~AsyncTaskDialog() {}

void AsyncTaskDialog::updateLog(const QString &log)
{
    ui->logTextEdit->setPlainText(log);
}

void AsyncTaskDialog::updateProgressTimer()
{
    const int secondsElapsed = (task->getElapsedTime() + 500) / 1000;
    const int minutesElapsed = secondsElapsed / 60;
    const int hoursElapsed = minutesElapsed / 60;

    QString label;
    if (hoursElapsed) {
        label += tr("%n hours", nullptr, hoursElapsed);
        label += " ";
    }
    if (minutesElapsed) {
        label += tr("%n minutes", nullptr, minutesElapsed % 60);
        label += " ";
    }
    label += tr("%n seconds", nullptr, secondsElapsed % 60);

    ui->timeLabel->setText(tr("Running for %1", "time").arg(label));
}

void AsyncTaskDialog::closeEvent(QCloseEvent *event)
{
    if (interruptOnClose) {
        task->interrupt();
        task->wait();
    }

    QWidget::closeEvent(event);
}

void AsyncTaskDialog::reject()
{
    task->interrupt();
}
