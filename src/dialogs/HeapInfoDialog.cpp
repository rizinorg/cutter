#include "HeapInfoDialog.h"
#include "ui_HeapInfoDialog.h"

HeapInfoDialog::HeapInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HeapInfoDialog)
{
    ui->setupUi(this);
}

HeapInfoDialog::~HeapInfoDialog()
{
    delete ui;
}
