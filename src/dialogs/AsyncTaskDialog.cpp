
#include "AsyncTaskDialog.h"
#include "utils/AsyncTask.h"

#include "ui_AsyncTaskDialog.h"


AsyncTaskDialog::AsyncTaskDialog(AsyncTask *task, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AsyncTaskDialog),
      task(task)
{
    ui->setupUi(this);

    QString title = task->getTitle();
    if (!title.isNull()) {
        setWindowTitle(title);
    }

    connect(task, &AsyncTask::logChanged, this, &AsyncTaskDialog::updateLog);
    connect(task, &AsyncTask::finished, this, [this]() {
        this->task = nullptr;
        close();
    });

    updateLog();

    connect(&timer, SIGNAL(timeout()), this, SLOT(updateProgressTimer()));
    timer.setInterval(1000);
    timer.setSingleShot(false);
    timer.start();

    updateProgressTimer();
}

AsyncTaskDialog::~AsyncTaskDialog()
{
}

void AsyncTaskDialog::updateLog()
{
    ui->logTextEdit->setPlainText(task->getLog());
}

void AsyncTaskDialog::updateProgressTimer()
{
    if(!task) {
        return;
    }

    int secondsElapsed = (task->getTimer().elapsed() + 500) / 1000;
    int minutesElapsed = secondsElapsed / 60;
    int hoursElapsed = minutesElapsed / 60;

    QString label = tr("Running for") + " ";
    if (hoursElapsed) {
        label += tr("%n hour", "%n hours", hoursElapsed);
        label += " ";
    }
    if (minutesElapsed) {
        label += tr("%n minute", "%n minutes", minutesElapsed % 60);
        label += " ";
    }
    label += tr("%n seconds", "%n second", secondsElapsed % 60);
    ui->timeLabel->setText(label);
}

void AsyncTaskDialog::closeEvent(QCloseEvent *event)
{
    if (interruptOnClose && task) {
        task->interrupt();
        task->wait();
    }

    QWidget::closeEvent(event);
}

void AsyncTaskDialog::reject()
{
    if (task) {
        task->interrupt();
    }
}
