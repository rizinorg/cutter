#include "R2TaskDialog.h"
#include "common/R2Task.h"

#include <QCloseEvent>

#include "ui_R2TaskDialog.h"

R2TaskDialog::R2TaskDialog(R2Task::Ptr task, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::R2TaskDialog),
      task(task)
{
    ui->setupUi(this);

    connect(task.data(), &R2Task::finished, this, [this]() {
        close();
    });

    connect(&timer, SIGNAL(timeout()), this, SLOT(updateProgressTimer()));
    timer.setInterval(1000);
    timer.setSingleShot(false);
    timer.start();

    elapsedTimer.start();
    updateProgressTimer();
}

R2TaskDialog::~R2TaskDialog()
{
}

void R2TaskDialog::updateProgressTimer()
{
    int secondsElapsed = elapsedTimer.elapsed() / 1000;
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

void R2TaskDialog::setDesc(const QString &label)
{
    ui->descLabel->setText(label);
}

void R2TaskDialog::closeEvent(QCloseEvent *event)
{
    if (breakOnClose) {
        task->breakTask();
        setDesc("Attempting to stop the task...");
        event->ignore();
    } else {
        QWidget::closeEvent(event);
    }
}

void R2TaskDialog::reject()
{
    task->breakTask();
    setDesc("Attempting to stop the task...");

}
