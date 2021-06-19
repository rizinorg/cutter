#include "HeapInfoDialog.h"
#include "ui_HeapInfoDialog.h"

HeapInfoDialog::HeapInfoDialog(RVA offset, QWidget *parent)
    : QDialog(parent), ui(new Ui::HeapInfoDialog), offset(offset)
{
    ui->setupUi(this);
    updateFields();
}

HeapInfoDialog::~HeapInfoDialog()
{
    delete ui;
}

void HeapInfoDialog::updateFields()
{
    this->setWindowTitle(QString("Chunk @ ") + RAddressString(offset));
    this->ui->baseEdit->setText(RAddressString(offset));
}
