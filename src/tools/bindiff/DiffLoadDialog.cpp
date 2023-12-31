#include "DiffLoadDialog.h"
#include "ui_DiffLoadDialog.h"

#include "DiffWaitDialog.h"

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

    ui->comboBoxAnalysis->addItem(tr("Basic"));
    ui->comboBoxAnalysis->addItem(tr("Auto"));
    ui->comboBoxAnalysis->addItem(tr("Experimental"));

    auto index = ui->comboBoxAnalysis->findData(tr("Auto"), Qt::DisplayRole);
    ui->comboBoxAnalysis->setCurrentIndex(index);
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

int DiffLoadDialog::getLevel() const
{
    return ui->comboBoxAnalysis->currentIndex();
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

void DiffLoadDialog::on_buttonBox_accepted()
{
    emit startDiffing();
}

void DiffLoadDialog::on_buttonBox_rejected() {}
