#include "dialogs/LoadNewTypesDialog.h"
#include "ui_LoadNewTypesDialog.h"

#include "Cutter.h"
#include "common/Configuration.h"
#include "widgets/TypesWidget.h"

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
        QMessageBox popup(this);
        popup.setWindowTitle(tr("Error"));
        popup.setText(file.errorString());
        popup.setInformativeText(tr("Do you want to try again?"));
        popup.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        popup.setDefaultButton(QMessageBox::Yes);
        if (popup.exec() == QMessageBox::Yes) {
            on_selectFileButton_clicked();
        }
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
    emit newTypesLoaded();
}
