#include "DiffLoadDialog.h"

#include "ui_DiffLoadDialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include <core/Cutter.h>
#include <rz_th.h>

DiffLoadDialog::DiffLoadDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DiffLoadDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setModal(true);

    ui->lineEditFileA->setReadOnly(true);
    ui->lineEditFileA->setText("");

    ui->lineEditFileB->setReadOnly(true);
    ui->lineEditFileB->setText("");

    ui->comboBoxAnalysis->addItem(tr("Basic"));
    ui->comboBoxAnalysis->addItem(tr("Auto"));
    ui->comboBoxAnalysis->addItem(tr("Experimental"));

    ui->comboBoxCompare->addItem(tr("Default"));
    ui->comboBoxCompare->addItem(tr("All functions"));
    ui->comboBoxCompare->addItem(tr("Only Symbols"));

    connect(ui->buttonFileAOpen, &QPushButton::clicked, this,
            &DiffLoadDialog::onButtonFileAOpenClicked);
    connect(ui->buttonFileBOpen, &QPushButton::clicked, this,
            &DiffLoadDialog::onButtonFileBOpenClicked);

    auto index = ui->comboBoxAnalysis->findData(tr("Auto"), Qt::DisplayRole);
    ui->comboBoxAnalysis->setCurrentIndex(index);
}

DiffLoadDialog::~DiffLoadDialog() {}

QString DiffLoadDialog::getFileA() const
{
    return ui->lineEditFileA->text();
}

QString DiffLoadDialog::getFileB() const
{
    return ui->lineEditFileB->text();
}

int DiffLoadDialog::getLevel() const
{
    return ui->comboBoxAnalysis->currentIndex();
}

int DiffLoadDialog::getCompare() const
{
    return ui->comboBoxCompare->currentIndex();
}

void DiffLoadDialog::onButtonFileAOpenClicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select File 2"));
    dialog.setNameFilters({ tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (fileName.isEmpty()) {
        return;
    }

    ui->lineEditFileB->setText(fileName);
}

void DiffLoadDialog::onButtonFileBOpenClicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select File 2"));
    dialog.setNameFilters({ tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (fileName.isEmpty()) {
        return;
    }

    ui->lineEditFileA->setText(fileName);
}

void DiffLoadDialog::onButtonBoxAccepted()
{
    // Check files exists
    emit startDiffing();
}

void DiffLoadDialog::onButtonBoxRejected() {}
