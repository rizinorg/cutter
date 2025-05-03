#include "RizinTaskDialog.h"
#include "common/RizinTask.h"

#include <QCloseEvent>

#include "ui_RizinTaskDialog.h"

RizinTaskDialog::RizinTaskDialog(RizinTask::Ptr task, QWidget *parent)
    : QDialog(parent), ui(new Ui::RizinTaskDialog), task(task)
{
    ui->setupUi(this);

    connect(task.data(), &RizinTask::finished, this, [this]() { close(); });

    connect(&timer, &QTimer::timeout, this, &RizinTaskDialog::updateProgressTimer);
    timer.setInterval(1000);
    timer.setSingleShot(false);
    timer.start();

    elapsedTimer.start();
    updateProgressTimer();
}

RizinTaskDialog::~RizinTaskDialog() {}

void RizinTaskDialog::updateProgressTimer()
{
    int secondsElapsed = elapsedTimer.elapsed() / 1000;
    int minutesElapsed = secondsElapsed / 60;
    int hoursElapsed = minutesElapsed / 60;

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

void RizinTaskDialog::setDesc(const QString &label)
{
    ui->descLabel->setText(label);
}

void RizinTaskDialog::closeEvent(QCloseEvent *event)
{
    if (breakOnClose) {
        task->breakTask();
        setDesc(tr("Attempting to stop the task..."));
        event->ignore();
    } else {
        QWidget::closeEvent(event);
    }
}

void RizinTaskDialog::reject()
{
    task->breakTask();
    setDesc(tr("Attempting to stop the task..."));
}
