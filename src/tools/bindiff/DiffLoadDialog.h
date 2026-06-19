#ifndef DIFF_LOAD_DIALOG_H
#define DIFF_LOAD_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>

#include <core/Cutter.h>
#include <memory>

namespace Ui {
class DiffLoadDialog;
}

class DiffLoadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiffLoadDialog(QWidget *parent = nullptr);
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
};

#endif // DIFF_LOAD_DIALOG_H
