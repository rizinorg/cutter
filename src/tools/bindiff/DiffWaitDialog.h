#ifndef DIFF_WAIT_DIALOG_H
#define DIFF_WAIT_DIALOG_H

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
    explicit DiffWaitDialog(BinDiff *bDiff, QWidget *parent = nullptr);
    ~DiffWaitDialog();

    void show(QString original, QString modified, int level, int compare);

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
    QTimer timer;
    BinDiff *bDiff;
    std::unique_ptr<Ui::DiffWaitDialog> ui;
};

#endif // DIFF_WAIT_DIALOG_H
