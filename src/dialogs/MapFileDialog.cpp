#include "MapFileDialog.h"
#include "ui_MapFileDialog.h"

#include "common/Configuration.h"

#include <QFileDialog>

MapFileDialog::MapFileDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::MapFileDialog)
{
    ui->setupUi(this);
}

MapFileDialog::~MapFileDialog() {}

void MapFileDialog::on_selectFileButton_clicked()
{
    QString currentDir = Config()->getRecentFolder();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"), currentDir);

    if (!fileName.isEmpty()) {
        ui->filenameLineEdit->setText(fileName);
        Config()->setRecentFolder(QFileInfo(fileName).absolutePath());
    }
}

void MapFileDialog::on_buttonBox_accepted()
{
    const QString &filePath = QDir::toNativeSeparators(ui->filenameLineEdit->text());
    RVA mapAddress = RVA_INVALID;
    QString mapAddressStr = ui->mapAddressLineEdit->text();
    if (!mapAddressStr.isEmpty()) {
        mapAddress = Core()->math(mapAddressStr);
    }

    if (!Core()->mapFile(filePath, mapAddress)) {
        QMessageBox::critical(this, tr("Map new file file"), tr("Failed to map a new file"));
        return;
    }
    close();
}

void MapFileDialog::on_buttonBox_rejected()
{
    close();
}
