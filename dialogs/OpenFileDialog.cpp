#include "OpenFileDialog.h"
#include "ui_OpenFileDialog.h"

#include "common/Configuration.h"

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
    QString currentDir = Config()->getRecentFolder();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"), currentDir);

    if (!fileName.isEmpty()) {
        ui->filenameLineEdit->setText(fileName);
        Config()->setRecentFolder(QFileInfo(fileName).absolutePath());
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

    if (!Core()->openFile(filePath, mapAddress)) {
        QMessageBox::critical(this, tr("Open file"), tr("Failed to open file"));
        return;
    }
    close();
}

void OpenFileDialog::on_buttonBox_rejected()
{
    close();
}
