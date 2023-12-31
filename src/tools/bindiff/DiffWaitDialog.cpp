#include "DiffWaitDialog.h"
#include "ui_DiffWaitDialog.h"

#include <QTime>

#include <core/Cutter.h>
#include <rz_util.h>

DiffWaitDialog::DiffWaitDialog(QWidget *parent)
    : QDialog(parent), timer(parent), bDiff(new BinDiff()), ui(new Ui::DiffWaitDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    ui->lineEditNFuncs->setReadOnly(true);
    ui->lineEditMatches->setReadOnly(true);
    ui->lineEditEstimatedTime->setReadOnly(true);
    ui->lineEditElapsedTime->setReadOnly(true);
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
    if (bDiff->isRunning()) {
        bDiff->cancel();
        bDiff->wait();
    }
}

QList<BinDiffMatchDescription> DiffWaitDialog::matches()
{
    return bDiff->matches();
}

QList<FunctionDescription> DiffWaitDialog::mismatch(bool fileA)
{
    return bDiff->mismatch(fileA);
}

void DiffWaitDialog::show(QString original, QString modified, int level)
{
    connect(this, &DiffWaitDialog::cancelJob, bDiff.get(), &BinDiff::cancel);
    connect(bDiff.get(), &BinDiff::progress, this, &DiffWaitDialog::onProgress);
    connect(bDiff.get(), &BinDiff::complete, this, &DiffWaitDialog::onCompletion);
    connect(&timer, &QTimer::timeout, this, &DiffWaitDialog::updateElapsedTime);

    ui->lineEditOriginal->setText(original);
    ui->lineEditModified->setText(modified);

    bDiff->setAnalysisLevel(level);
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
}
