#include "SetToDataDialog.h"
#include "ui_SetToDataDialog.h"
#include <QIntValidator>

SetToDataDialog::SetToDataDialog(RVA startAddr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetToDataDialog),
    startAddress(startAddr)
{
    ui->setupUi(this);
    auto validator = new QIntValidator(this);
    validator->setBottom(1);
    ui->sizeEdit->setValidator(validator);
    ui->repeatEdit->setValidator(validator);
    ui->startAddrLabel->setText(RAddressString(startAddr));
    updateEndAddress();
}

SetToDataDialog::~SetToDataDialog()
{
    delete ui;
}

int SetToDataDialog::getItemSize()
{
    return ui->sizeEdit->text().toInt();
}

int SetToDataDialog::getItemCount()
{
    return ui->repeatEdit->text().toInt();
}

void SetToDataDialog::updateEndAddress()
{
    RVA endAddr = startAddress + (getItemSize() * getItemCount());
    ui->endAddrLabel->setText(RAddressString(endAddr));
}

void SetToDataDialog::on_sizeEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateEndAddress();
}

void SetToDataDialog::on_repeatEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateEndAddress();
}
