#include "NativeDebugDialog.h"
#include "ui_NativeDebugDialog.h"

#include <QMessageBox>

NativeDebugDialog::NativeDebugDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NativeDebugDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

NativeDebugDialog::~NativeDebugDialog() {}

void NativeDebugDialog::on_buttonBox_accepted()
{
}

void NativeDebugDialog::on_buttonBox_rejected()
{
    close();
}

QString NativeDebugDialog::getArgs() const
{
    return ui->argEdit->text();
}
