#include "DiffLoadDialog.h"

#include "ui_DiffLoadDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <core/Cutter.h>
#include <rz_th.h>

DiffLoadDialog::DiffLoadDialog(BinDiff *bDiff, QWidget *parent)
    : QDialog(parent), bDiff(bDiff), ui(new Ui::DiffLoadDialog)
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
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DiffLoadDialog::onButtonBoxAccepted);

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

    const QFileInfo info(fileName);

    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, tr("Invalid Path"),
                             tr("Given file path for File A is not valid."));
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
    const QFileInfo info(fileName);

    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, tr("Invalid Path"),
                             tr("Given file path for File B is not valid."));
        return;
    }

    ui->lineEditFileA->setText(fileName);
}

void DiffLoadDialog::onButtonBoxAccepted()
{
    if (ui->lineEditFileA->text().isEmpty()) {
        QMessageBox::warning(this, tr("Empty FileA"), tr("Select a file for diffing."));
        return;
    }
    if (ui->lineEditFileB->text().isEmpty()) {
        QMessageBox::warning(this, tr("Empty FileB"), tr("Select a file for diffing."));
        return;
    }
    auto waitDialog = new DiffWaitDialog(bDiff, this);
    waitDialog->show(ui->lineEditFileA->text(), ui->lineEditFileB->text(),
                     ui->comboBoxAnalysis->currentIndex(), ui->comboBoxCompare->currentIndex());
    printf("hello world");
    emit startDiffing();
}

void DiffLoadDialog::onButtonBoxRejected() {}
