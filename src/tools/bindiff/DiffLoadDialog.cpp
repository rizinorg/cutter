#include "DiffLoadDialog.h"
#include "ui_DiffLoadDialog.h"

#include <core/Cutter.h>
#include <rz_th.h>

#include <QFileDialog>
#include <QMessageBox>

DiffLoadDialog::DiffLoadDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DiffLoadDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    ui->lineEditFileName->setReadOnly(true);
    ui->lineEditFileName->setText("");

    ui->lineEditDiffFile->setReadOnly(true);
    ui->lineEditDiffFile->setText("");
}

DiffLoadDialog::~DiffLoadDialog() {}

QString DiffLoadDialog::getFileToOpen() const
{
    return ui->lineEditFileName->text();
}

QString DiffLoadDialog::getPreviousDiffFile() const
{
    return ui->lineEditDiffFile->text();
}

void DiffLoadDialog::on_buttonDiffOpen_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select the previous diff to load"));
    dialog.setNameFilters({ tr("JSON array (*.json)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (fileName.isEmpty()) {
        return;
    }

    ui->lineEditDiffFile->setText(fileName);
}

void DiffLoadDialog::on_buttonFileOpen_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select a file to use for diffing"));
    dialog.setNameFilters({ tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (fileName.isEmpty()) {
        return;
    }

    ui->lineEditFileName->setText(fileName);
}

void DiffLoadDialog::openErrorBox(QString errorMessage)
{
    QMessageBox mb(this);
    mb.setIcon(QMessageBox::Warning);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setWindowTitle(tr("Error"));
    mb.setText(errorMessage);
    mb.exec();
}

void DiffLoadDialog::on_buttonBox_accepted()
{
    QString fileName = getFileToOpen();
    if (fileName.isEmpty()) {
        openErrorBox(tr("The compare file was not selected."));
        return;
    }
}

void DiffLoadDialog::on_buttonBox_rejected()
{
    // cancel
}
