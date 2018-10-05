#include "HexdumpRangeDialog.h"
#include "ui_HexdumpRangeDialog.h"

HexdumpRangeDialog::HexdumpRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HexdumpRangeDialog)
{
    ui->setupUi(this);
}

HexdumpRangeDialog::~HexdumpRangeDialog()
{
    delete ui;
}

QString HexdumpRangeDialog::getStartAddress()
{
    return ui->startAddressLineEdit->text();
}

QString HexdumpRangeDialog::getEndAddress()
{
    return ui->endAddressLineEdit->text();
}

QString HexdumpRangeDialog::getLength()
{
    return ui->lengthLineEdit->text();
}

bool HexdumpRangeDialog::getEndAddressRadioButtonChecked()
{
    return ui->endAddressRadioButton->isChecked();
}

bool HexdumpRangeDialog::getLengthRadioButtonChecked()
{
    return ui->lengthRadioButton->isChecked();
}
