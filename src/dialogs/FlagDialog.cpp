#include "FlagDialog.h"
#include "ui_FlagDialog.h"

FlagDialog::FlagDialog(RVA offset, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlagDialog),
    offset(offset),
    core(Core())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    auto size_validator = new QIntValidator(ui->sizeEdit);
    size_validator->setBottom(1);
    ui->sizeEdit->setValidator(size_validator);
}

FlagDialog::~FlagDialog() {}

void FlagDialog::on_buttonBox_accepted()
{
    QString name = ui->nameEdit->text();
    RVA size = ui->sizeEdit->text().toULongLong();
    core->addFlag(offset, name, size);
}

void FlagDialog::on_buttonBox_rejected()
{
    close();
}
