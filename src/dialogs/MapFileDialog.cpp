#include "MapFileDialog.h"

#include "common/Configuration.h"
#include "ui_MapFileDialog.h"

#include <QFileDialog>

MapFileDialog::MapFileDialog(QWidget *parent) : QDialog(parent), ui(new Ui::MapFileDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &MapFileDialog::onButtonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &MapFileDialog::onButtonBoxRejected);
    connect(ui->selectFileButton, &QPushButton::clicked, this,
            &MapFileDialog::onSelectFileButtonClicked);
}

MapFileDialog::~MapFileDialog() {}

void MapFileDialog::onSelectFileButtonClicked()
{
    const QString currentDir = Config()->getRecentFolder();
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"), currentDir);

    if (!fileName.isEmpty()) {
        ui->filenameLineEdit->setText(fileName);
        Config()->setRecentFolder(QFileInfo(fileName).absolutePath());
    }
}

void MapFileDialog::onButtonBoxAccepted()
{
    const QString &filePath = QDir::toNativeSeparators(ui->filenameLineEdit->text());
    RVA mapAddress = RVA_INVALID;
    const QString mapAddressStr = ui->mapAddressLineEdit->text();
    if (!mapAddressStr.isEmpty()) {
        mapAddress = Core()->math(mapAddressStr);
    }

    if (!Core()->mapFile(filePath, mapAddress)) {
        QMessageBox::critical(this, tr("Map new file"), tr("Failed to map a new file"));
        return;
    }
    close();
}

void MapFileDialog::onButtonBoxRejected()
{
    close();
}
