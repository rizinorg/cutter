#include "HexdumpRangeDialog.h"
#include "ui_HexdumpRangeDialog.h"

HexdumpRangeDialog::HexdumpRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HexdumpRangeDialog)
{
    ui->setupUi(this);

    //subscribe to a text change slot
    connect(ui->endAddressLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);
    connect(ui->lengthLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);

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

void HexdumpRangeDialog::textEdited()
{
    QString startAddress = ui->startAddressLineEdit->text();
    ut64 endAddress;
    ut64 length;

    if (sender() == ui->endAddressLineEdit) {
        length = Core()->math(getEndAddress() + " - " + startAddress);

        ui->lengthLineEdit->setText(
            QString("0x%1").arg(length, 0, 16));
    } else if ( sender() == ui->lengthLineEdit) {
        //we edited the length, so update the end address to be start address + length
        endAddress = Core()->math(startAddress + " + " + getLength());
        ui->endAddressLineEdit->setText(
            QString("0x%1").arg(endAddress, 0, 16));
    }

    return;
}
