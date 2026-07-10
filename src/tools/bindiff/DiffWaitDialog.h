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
    explicit DiffWaitDialog(QWidget *parent = nullptr);
    ~DiffWaitDialog();

    void show(const QString &original, const QString &modified, int level, int compare);

public slots:
    void onProgress(BinDiffStatusDescription status);
    void onCompletion();
    void updateElapsedTime();

signals:
    void cancelJob();

private slots:
    void onButtonBoxRejected();

private:
    QElapsedTimer eTimer;
    std::unique_ptr<BinDiff> bDiff;
    QTimer timer;
    std::unique_ptr<Ui::DiffWaitDialog> ui;
};

#endif // DIFF_WAIT_DIALOG_H
