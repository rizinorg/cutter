#include "RenameDialog.h"
#include "ui_RenameDialog.h"

RenameDialog::RenameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RenameDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

RenameDialog::~RenameDialog() {}

void RenameDialog::on_buttonBox_accepted()
{
    // Rename function and refresh widgets
    QString name = ui->nameEdit->text();
}

void RenameDialog::on_buttonBox_rejected()
{
    close();
}

void RenameDialog::setName(QString fcnName)
{
    ui->nameEdit->setText(fcnName);
}

QString RenameDialog::getName() const
{
    return ui->nameEdit->text();
}
