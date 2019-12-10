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

QString NativeDebugDialog::getArgs() const
{
    return ui->argEdit->toPlainText();
}

void NativeDebugDialog::setArgs(const QString &args)
{
    ui->argEdit->setText(args);
    ui->argEdit->selectAll();
}
