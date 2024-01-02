#include "DiffWaitDialog.h"
#include "ui_DiffWaitDialog.h"

#include <QTime>

#include <core/Cutter.h>
#include <rz_util.h>

#include "DiffWindow.h"

DiffWaitDialog::DiffWaitDialog(QWidget *parent)
    : QDialog(parent), timer(parent), bDiff(new BinDiff()), ui(new Ui::DiffWaitDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setModal(true);

    ui->lineEditNFuncs->setReadOnly(true);
    ui->lineEditMatches->setReadOnly(true);
    ui->lineEditOriginal->setReadOnly(true);
    ui->lineEditModified->setReadOnly(true);
    ui->progressBar->setValue(0);
    ui->lineEditNFuncs->setText("0");
    ui->lineEditMatches->setText("0");

    QTime zero(0, 0, 0, 0);
    ui->lineEditElapsedTime->setText(zero.toString("hh:mm:ss"));
    ui->lineEditEstimatedTime->setText(zero.toString("hh:mm:ss"));
}

DiffWaitDialog::~DiffWaitDialog()
{
    if (bDiff && bDiff->isRunning()) {
        bDiff->cancel();
        bDiff->wait();
    }
    delete bDiff;
}

void DiffWaitDialog::show(QString original, QString modified, int level, int compare)
{
    connect(this, &DiffWaitDialog::cancelJob, bDiff, &BinDiff::cancel);
    connect(bDiff, &BinDiff::progress, this, &DiffWaitDialog::onProgress);
    connect(bDiff, &BinDiff::complete, this, &DiffWaitDialog::onCompletion);
    connect(&timer, &QTimer::timeout, this, &DiffWaitDialog::updateElapsedTime);

    ui->lineEditOriginal->setText(original);
    ui->lineEditModified->setText(modified);

    bDiff->setAnalysisLevel(level);
    bDiff->setCompareLogic(compare);
    bDiff->setFile(modified);
    eTimer.restart();
    timer.setSingleShot(false);
    timer.start(1000);

    bDiff->start();
    this->QDialog::show();
}

void DiffWaitDialog::onProgress(BinDiffStatusDescription status)
{
    int partial = status.total - status.nLeft;
    ut32 progress = (100 * partial) / status.total;
    ui->progressBar->setValue(progress);
    ui->lineEditNFuncs->setText(QString::asprintf("%lu", status.nLeft));
    ui->lineEditMatches->setText(QString::asprintf("%lu", status.nMatch));

    double speed = ((double)partial) / ((double)eTimer.elapsed());
    ut64 seconds = (((double)status.nLeft) / speed) / 1000ull;
    int hours = seconds / 3600;
    seconds -= (hours * 3600);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    QTime estimated(hours, minutes, seconds, 0);
    ui->lineEditEstimatedTime->setText(estimated.toString("hh:mm:ss"));
}

void DiffWaitDialog::onCompletion()
{
    timer.stop();

    if (bDiff->hasData()) {
        auto results = new DiffWindow(bDiff, parentWidget());
        bDiff = nullptr;
        results->showMaximized();
    }

    close();
}

void DiffWaitDialog::updateElapsedTime()
{
    ut64 seconds = eTimer.elapsed() / 1000ull;
    int hours = seconds / 3600;
    seconds -= (hours * 3600);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    QTime current(hours, minutes, seconds, 0);
    ui->lineEditElapsedTime->setText(current.toString("hh:mm:ss"));
}

void DiffWaitDialog::on_buttonBox_rejected()
{
    timer.stop();
    emit cancelJob();
    close();
}
