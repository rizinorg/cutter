#include "EditFunctionDialog.h"
#include "ui_EditFunctionDialog.h"

EditFunctionDialog::EditFunctionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditFunctionDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

EditFunctionDialog::~EditFunctionDialog() {}

QString EditFunctionDialog::getNameText()
{
    QString ret = ui->nameLineEdit->text();
    return ret;
}

void EditFunctionDialog::setNameText(const QString &name)
{
    ui->nameLineEdit->setText(name);
}

QString EditFunctionDialog::getStartAddrText()
{
    QString ret = ui->startLineEdit->text();
    return ret;
}

void EditFunctionDialog::setStartAddrText(const QString &startAddr)
{
    ui->startLineEdit->setText(startAddr);
}

QString EditFunctionDialog::getEndAddrText()
{
    QString ret = ui->endLineEdit->text();
    return ret;
}

void EditFunctionDialog::setEndAddrText(const QString &endAddr)
{
    ui->endLineEdit->setText(endAddr);
}

QString EditFunctionDialog::getStackSizeText()
{
    QString ret = ui->stackSizeLineEdit->text();
    return ret;
}

void EditFunctionDialog::setStackSizeText(const QString &stackSize)
{
    ui->stackSizeLineEdit->setText(stackSize);
}

void EditFunctionDialog::setCallConList(const QStringList callConList) {
    ui->callConComboBox->addItems(callConList);
}

void EditFunctionDialog::setCallConSelected(const QString selected) {
    ui->callConComboBox->setCurrentText(selected);
}

QString EditFunctionDialog::getCallConSelected() {
    return ui->callConComboBox->currentText();
}

void EditFunctionDialog::on_buttonBox_accepted()
{
}

void EditFunctionDialog::on_buttonBox_rejected()
{
    close();
}
