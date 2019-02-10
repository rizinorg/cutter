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
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

LoadNewTypesDialog::~LoadNewTypesDialog() {}

void LoadNewTypesDialog::on_selectFileButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select file"), Config()->getRecentFolder(), "*.h");
    if (filename.isEmpty()) {
        return;
    }
    Config()->setRecentFolder(QFileInfo(filename).absolutePath());
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox popup(this);
        popup.setWindowTitle(tr("Error"));
        popup.setText(file.errorString());
        popup.setStandardButtons(QMessageBox::Ok);
        popup.exec();
        on_selectFileButton_clicked();
        return;
    }
    ui->filenameLineEdit->setText(filename);
    ui->plainTextEdit->setPlainText(file.readAll());
}

void LoadNewTypesDialog::on_plainTextEdit_textChanged()
{
    if (ui->plainTextEdit->toPlainText().isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void LoadNewTypesDialog::done(int r)
{
    if (r == QDialog::Accepted) {
        QString error = Core()->addTypes(ui->plainTextEdit->toPlainText());
        if (error.isEmpty()) {
            emit newTypesLoaded();
            QDialog::done(r);
            return;
        }

        QMessageBox popup(this);
        popup.setWindowTitle(tr("Error"));
        popup.setText(tr("There was some error while loading new types"));
        popup.setDetailedText(error);
        popup.setStandardButtons(QMessageBox::Ok);
        popup.exec();
    } else {
        QDialog::done(r);
    }
}
