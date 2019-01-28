#include "OpenFileDialog.h"
#include "ui_OpenFileDialog.h"

#include <QFileDialog>
#include <QSettings>

OpenFileDialog::OpenFileDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::OpenFileDialog)
{
    ui->setupUi(this);
}

OpenFileDialog::~OpenFileDialog() {}

void OpenFileDialog::on_selectFileButton_clicked()
{
    QSettings settings;
    QString currentDir = settings.value("recentFolder", QDir::homePath()).toString();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"), currentDir);

    if (!fileName.isEmpty()) {
        ui->filenameLineEdit->setText(fileName);
        settings.setValue("recentFolder", QDir::toNativeSeparators(QFileInfo(fileName).absolutePath()));
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
    Core()->openFile(filePath, mapAddress);
}

void OpenFileDialog::on_buttonBox_rejected()
{
    close();
}
