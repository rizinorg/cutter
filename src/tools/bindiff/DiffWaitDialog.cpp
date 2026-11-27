#include "DiffWaitDialog.h"

#include "ui_DiffWaitDialog.h"

#include <QTime>

#include <core/Cutter.h>
#include <rz_util.h>

DiffWaitDialog::DiffWaitDialog(CutterDiff *cutterDiff, const BinDiffOptions &options,
                               QWidget *parent)
    : QDialog(parent),
      ui(new Ui::DiffWaitDialog),
      cutterDiff(cutterDiff),
      options(options),
      timer(parent)
{
    Q_ASSERT(cutterDiff != nullptr);
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setModal(true);
    bDiff.reset(new BinDiff(cutterDiff, options));

    ui->lineEditOriginal->setReadOnly(true);
    ui->lineEditModified->setReadOnly(true);
    ui->progressBar->setValue(0);

    const QTime zero(0, 0, 0, 0);
    ui->lineEditElapsedTime->setText(zero.toString("hh:mm:ss"));
    ui->lineEditEstimatedTime->setText(zero.toString("hh:mm:ss"));
}

DiffWaitDialog::~DiffWaitDialog()
{
    if (bDiff && bDiff->isRunning()) {
        bDiff->cancel();
        bDiff->wait();
    }
}

void DiffWaitDialog::show(const QString &original, const QString &modified)
{
    connect(this, &DiffWaitDialog::cancelJob, bDiff.get(), &BinDiff::cancel);
    connect(bDiff.get(), &BinDiff::progress, this, &DiffWaitDialog::onProgress);
    connect(bDiff.get(), &BinDiff::complete, this, &DiffWaitDialog::onCompletion);
    connect(&timer, &QTimer::timeout, this, &DiffWaitDialog::updateElapsedTime);

    ui->lineEditOriginal->setText(original);
    ui->lineEditModified->setText(modified);

    eTimer.restart();
    timer.setSingleShot(false);
    timer.start(1000);

    bDiff->start();
    this->QDialog::show();
}

void DiffWaitDialog::onProgress(BinDiffStatusDescription status)
{
    if (status.total <= 0) {
        ui->progressBar->setValue(0);
        ui->lineEditEstimatedTime->clear();
        return;
    }

    const double completed = std::clamp(status.nLeft, 0.0, double(status.total));

    const int progress = static_cast<int>((completed / double(status.total)) * 100.0);

    ui->progressBar->setValue(progress);

    const qint64 elapsed = eTimer.elapsed();

    if (completed > 0.0 && elapsed > 0) {
        const double speed = completed / double(elapsed);

        const double remaining = double(status.total) - completed;

        const auto remainingMs = static_cast<qint64>(remaining / speed);

        const qint64 seconds = remainingMs / 1000;

        const int hours = seconds / 3600;
        const int minutes = (seconds % 3600) / 60;
        const int secs = seconds % 60;

        const QTime estimated(hours, minutes, secs, 0);
        ui->lineEditEstimatedTime->setText(estimated.toString("hh:mm:ss"));
    } else {
        ui->lineEditEstimatedTime->clear();
    }
}

void DiffWaitDialog::onCompletion()
{
    timer.stop();
    accept();
}

void DiffWaitDialog::updateElapsedTime()
{
    ut64 seconds = eTimer.elapsed() / 1000ull;
    const int hours = seconds / 3600;
    seconds -= (hours * 3600);
    const int minutes = seconds / 60;
    seconds = seconds % 60;
    const QTime current(hours, minutes, seconds, 0);
    ui->lineEditElapsedTime->setText(current.toString("hh:mm:ss"));
}

void DiffWaitDialog::onButtonBoxRejected()
{
    timer.stop();
    emit cancelJob();
    close();
}
