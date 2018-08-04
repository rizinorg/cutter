#ifndef SETTODATADIALOG_H
#define SETTODATADIALOG_H

#include <QDialog>
#include "Cutter.h"

namespace Ui {
class SetToDataDialog;
}

class SetToDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetToDataDialog(RVA startAddr, QWidget *parent = nullptr);
    ~SetToDataDialog();

    int getItemSize();
    int getItemCount();

private slots:
    void on_sizeEdit_textChanged(const QString &arg1);
    void on_repeatEdit_textChanged(const QString &arg1);

private:
    void updateEndAddress();

    Ui::SetToDataDialog *ui;
    RVA startAddress;
};

#endif // SETTODATADIALOG_H
