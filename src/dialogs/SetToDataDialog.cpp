#include "SetToDataDialog.h"

#include "ui_SetToDataDialog.h"

#include <QIntValidator>

SetToDataDialog::SetToDataDialog(RVA startAddr, QWidget *parent)
    : QDialog(parent), ui(new Ui::SetToDataDialog), startAddress(startAddr)
{
    ui->setupUi(this);
    auto validator = new QIntValidator(this);
    validator->setBottom(1);
    ui->sizeEdit->setValidator(validator);
    ui->repeatEdit->setValidator(validator);
    ui->startAddrLabel->setText(rzAddressString(startAddr));
    updateEndAddress();

    connect(ui->sizeEdit, &QLineEdit::textChanged, this, &SetToDataDialog::onSizeEditTextChanged);
    connect(ui->repeatEdit, &QLineEdit::textChanged, this,
            &SetToDataDialog::onRepeatEditTextChanged);
}

SetToDataDialog::~SetToDataDialog() {}

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
    const RVA endAddr = startAddress + (getItemSize() * getItemCount());
    ui->endAddrLabel->setText(rzAddressString(endAddr));
}

void SetToDataDialog::onSizeEditTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateEndAddress();
}

void SetToDataDialog::onRepeatEditTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateEndAddress();
}
