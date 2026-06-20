#ifndef DIFF_LOAD_DIALOG_H
#define DIFF_LOAD_DIALOG_H

#include "DiffWaitDialog.h"

#include <QDialog>
#include <QListWidgetItem>

#include <BinDiff.h>
#include <core/Cutter.h>
#include <memory>

namespace Ui {
class DiffLoadDialog;
}

class DiffLoadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiffLoadDialog(BinDiff *bDiff, QWidget *parent = nullptr);
    ~DiffLoadDialog();

    QString getFileA() const;
    QString getFileB() const;
    int getLevel() const;
    int getCompare() const;

signals:
    void startDiffing();

private slots:
    void onButtonFileAOpenClicked();
    void onButtonFileBOpenClicked();
    void onButtonBoxAccepted();
    void onButtonBoxRejected();

private:
    std::unique_ptr<Ui::DiffLoadDialog> ui;
    BinDiff *bDiff;
};

#endif // DIFF_LOAD_DIALOG_H
