#include "HexdumpRangeDialog.h"
#include "ui_HexdumpRangeDialog.h"

HexdumpRangeDialog::HexdumpRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HexdumpRangeDialog)
{
    ui->setupUi(this);
    QRegExpValidator *v = new QRegExpValidator(QRegExp("(?:0[xX])?[0-9a-fA-F]+"), this);
    ui->lengthLineEdit->setValidator(v);
    ui->startAddressLineEdit->setValidator(v);
    ui->endAddressLineEdit->setValidator(v);

    //subscribe to a text change slot
    connect(ui->endAddressLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);
    connect(ui->lengthLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);
    connect(ui->endAddressRadioButton, &QRadioButton::clicked, this,
            &HexdumpRangeDialog::on_radioButtonClicked);
    connect(ui->lengthRadioButton, &QRadioButton::clicked, this,
            &HexdumpRangeDialog::on_radioButtonClicked);

}

HexdumpRangeDialog::~HexdumpRangeDialog()
{
    delete ui;
}

QString HexdumpRangeDialog::getStartAddress() const
{
    return ui->startAddressLineEdit->text();
}

QString HexdumpRangeDialog::getEndAddress() const
{
    return ui->endAddressLineEdit->text();
}

QString HexdumpRangeDialog::getLength() const
{
    return ui->lengthLineEdit->text();
}

bool HexdumpRangeDialog::getEndAddressRadioButtonChecked() const
{
    return ui->endAddressRadioButton->isChecked();
}

bool HexdumpRangeDialog::getLengthRadioButtonChecked() const
{
    return ui->lengthRadioButton->isChecked();
}

void HexdumpRangeDialog::setStartAddress(ut64 start)
{
    ui->startAddressLineEdit->setText(
        QString("0x%1").arg(start, 0, 16));
}

void HexdumpRangeDialog::textEdited()
{
    bool warningVisibile = false;
    ut64 startAddress = Core()->math(ui->startAddressLineEdit->text());
    ut64 endAddress = 0;
    ut64 length = 0;
    if (sender() == ui->endAddressLineEdit) {
        endAddress = Core()->math(getEndAddress());
        if (endAddress > startAddress) {
            length = endAddress - startAddress;
            ui->lengthLineEdit->setText(
                QString("0x%1").arg(length, 0, 16));
        }
        else {
            ui->lengthLineEdit->setText("Invalid");
        }
    } else if ( sender() == ui->lengthLineEdit) {
        //we edited the length, so update the end address to be start address + length
        length = Core()->math(getLength());
        endAddress = startAddress + length;
        ui->endAddressLineEdit->setText(
            QString("0x%1").arg(endAddress, 0, 16));
    }

    length = Core()->math(getLength());

    // Warn the user for potentially heavy operation
    if (length > 0x25000) {
        warningVisibile = true;
    }

    ui->selectionWarningLabel->setVisible(warningVisibile);

}

void HexdumpRangeDialog::on_radioButtonClicked(bool checked)
{

    if (sender() == ui->endAddressRadioButton && checked == true) {
        ui->lengthLineEdit->setEnabled(false);
        ui->endAddressLineEdit->setEnabled(true);
        ui->endAddressLineEdit->setFocus();
    } else if (sender() == ui->lengthRadioButton && checked == true) {
        ui->lengthLineEdit->setEnabled(true);
        ui->endAddressLineEdit->setEnabled(false);
        ui->lengthLineEdit->setFocus();
    }

}
