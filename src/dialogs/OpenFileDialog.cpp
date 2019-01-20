#include "OpenFileDialog.h"
#include "ui_OpenFileDialog.h"

#include <QFileDialog>

OpenFileDialog::OpenFileDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::OpenFileDialog)
{
    ui->setupUi(this);
}

OpenFileDialog::~OpenFileDialog() {}

void OpenFileDialog::on_selectFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"), QDir::homePath());

    if (!fileName.isEmpty()) {
        ui->filenameLineEdit->setText(fileName);
    }
}

void OpenFileDialog::on_buttonBox_accepted()
{
    const QString &filePath = QDir::toNativeSeparators(ui->filenameLineEdit->text());
    RVA mapAddress = RVA_INVALID;
    QString mapAddressStr = ui->mapAddressLineEdit->text();
    if (!mapAddressStr.isEmpty()) {
        mapAddress = Core()->math(mapAddressStr);
    }

    if(filePath.contains(';') || filePath.contains('|')){
        QMessageBox msgBox(this);
        msgBox.setText(tr("File name contains invalid characters"));
        msgBox.exec();
        return;
    }

    Core()->openFile(filePath, mapAddress);
    close();
}

void OpenFileDialog::on_buttonBox_rejected()
{
    close();
}
