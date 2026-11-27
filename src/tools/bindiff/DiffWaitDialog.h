#ifndef DIFF_WAIT_DIALOG_H
#define DIFF_WAIT_DIALOG_H

#include "CutterDiffWindow.h"

#include <QDialog>
#include <QElapsedTimer>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QTimer>

#include <BinDiff.h>
#include <core/Cutter.h>
#include <memory>

namespace Ui {
class DiffWaitDialog;
}

class DiffWaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiffWaitDialog(CutterDiff *cutterDiff, const BinDiffOptions &options,
                            QWidget *parent = nullptr);
    ~DiffWaitDialog();

    void show(const QString &original, const QString &modified);

public slots:
    void onProgress(BinDiffStatusDescription status);
    void onCompletion();
    void updateElapsedTime();

signals:
    void cancelJob();

private slots:
    void onButtonBoxRejected();

private:
    std::unique_ptr<Ui::DiffWaitDialog> ui;
    CutterDiff *cutterDiff = nullptr;
    const BinDiffOptions options;
    QTimer timer;
    std::unique_ptr<BinDiff> bDiff;
    QElapsedTimer eTimer;
};

#endif // DIFF_WAIT_DIALOG_H
