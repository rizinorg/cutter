#include "renamedialog.h"
#include "ui_renamedialog.h"

RenameDialog::RenameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RenameDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

RenameDialog::~RenameDialog()
{
    delete ui;
}

void RenameDialog::on_buttonBox_accepted()
{
    // Rename function and refresh widgets
    QString name = ui->nameEdit->text();
}

void RenameDialog::on_buttonBox_rejected()
{
    close();
}

void RenameDialog::setFunctionName(QString fcnName)
{
    ui->nameEdit->setText(fcnName);
}

QString RenameDialog::getFunctionName() const
{
    return ui->nameEdit->text();
}
