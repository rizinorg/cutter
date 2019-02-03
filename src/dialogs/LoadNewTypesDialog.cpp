#include "dialogs/LoadNewTypesDialog.h"
#include "ui_LoadNewTypesDialog.h"

#include "Cutter.h"
#include "common/Configuration.h"

#include <QFileDialog>
#include <QTemporaryFile>

LoadNewTypesDialog::LoadNewTypesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadNewTypesDialog)
{
    ui->setupUi(this);
    ui->plainTextEdit->setPlainText("");
}

LoadNewTypesDialog::~LoadNewTypesDialog() {}

void LoadNewTypesDialog::on_selectFileButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select file"), Config()->getRecentFolder(), "*.h");
    ui->filenameLineEdit->setText(filename);
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file";
        return;
    }
    ui->plainTextEdit->setPlainText(file.readAll());
    Config()->setRecentFolder(QFileInfo(filename).absolutePath());
}

void LoadNewTypesDialog::on_buttonBox_accepted()
{
    QTemporaryFile file(QDir(QDir::tempPath()).filePath("fileXXXXXX.h"));
    if (!file.open()) {
        qWarning() << "Unable to open file";
        return;
    }
    QTextStream fileOut(&file);
    fileOut << ui->plainTextEdit->toPlainText();
    file.close();
    Core()->cmd("to " + file.fileName());
    Core()->refreshAll();
}
