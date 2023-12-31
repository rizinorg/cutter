#ifndef DIFF_WAIT_DIALOG_H
#define DIFF_WAIT_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

#include <core/Cutter.h>

namespace Ui {
class DiffWaitDialog;
}

class DiffWaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiffWaitDialog(QWidget *parent = nullptr);
    ~DiffWaitDialog();

    void show(QString original, QString modified, int level);

    QList<BinDiffMatchDescription> matches();
    QList<FunctionDescription> mismatch(bool fileA);

public slots:
    void onProgress(BinDiffStatusDescription status);
    void onCompletion();
    void updateElapsedTime();

signals:
    void cancelJob();

private slots:
    void on_buttonBox_rejected();

private:
    QElapsedTimer eTimer;
    QTimer timer;
    std::unique_ptr<BinDiff> bDiff;
    std::unique_ptr<Ui::DiffWaitDialog> ui;
};

#endif // DIFF_WAIT_DIALOG_H
