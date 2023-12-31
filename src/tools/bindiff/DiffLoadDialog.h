#ifndef DIFF_LOAD_DIALOG_H
#define DIFF_LOAD_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

#include <core/Cutter.h>

namespace Ui {
class DiffLoadDialog;
}

class DiffLoadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiffLoadDialog(QWidget *parent = nullptr);
    ~DiffLoadDialog();

    QString getFileToOpen() const;
    QString getPreviousDiffFile() const;
    int getLevel() const;

signals:
    void startDiffing();

private slots:
    void on_buttonDiffOpen_clicked();
    void on_buttonFileOpen_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::DiffLoadDialog> ui;
};

#endif // DIFF_LOAD_DIALOG_H
