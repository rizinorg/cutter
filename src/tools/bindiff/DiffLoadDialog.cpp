#include "DiffLoadDialog.h"

#include "ui_DiffLoadDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <BinDiff.h>
#include <core/Cutter.h>
#include <rz_th.h>

DiffLoadDialog::DiffLoadDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DiffLoadDialog)
{
    cutterDiff.reset(new CutterDiff());
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setModal(true);

    // ui->lineEditFileA->setReadOnly(true);
    ui->lineEditFileA->setText("");

    // ui->lineEditFileB->setReadOnly(true);
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
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
            &DiffLoadDialog::onButtonBoxAccepted);
    connect(ui->setCurrentA, &QCheckBox::toggled, this, &DiffLoadDialog::onSetCurrentAChanged);
    connect(ui->setCurrentB, &QCheckBox::toggled, this, &DiffLoadDialog::onSetCurrentBChanged);

    auto index = ui->comboBoxAnalysis->findData(tr("Auto"), Qt::DisplayRole);
    ui->comboBoxAnalysis->setCurrentIndex(index);
}

DiffLoadDialog::~DiffLoadDialog() {}

void DiffLoadDialog::onSetCurrentAChanged(int state)
{
    if (state) {
        ui->lineEditFileA->setText(Core()->getConfig("file.path"));
        ui->setCurrentB->setDisabled(true);
        ui->buttonFileAOpen->setDisabled(true);
    } else {
        ui->lineEditFileA->setText("");
        ui->setCurrentB->setDisabled(false);
        ui->buttonFileAOpen->setDisabled(false);
    }
}

void DiffLoadDialog::onSetCurrentBChanged(int state)
{
    if (state) {
        ui->lineEditFileB->setText(Core()->getConfig("file.path"));
        ui->setCurrentA->setDisabled(true);
        ui->buttonFileBOpen->setDisabled(true);
    } else {
        ui->lineEditFileB->setText("");
        ui->setCurrentA->setDisabled(false);
        ui->buttonFileBOpen->setDisabled(false);
    }
}

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
    dialog.setWindowTitle(tr("Select File A"));
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
        QMessageBox::warning(this, tr("Invalid File Path"),
                             tr("Please provide a valid path for File A"));
        return;
    }

    ui->lineEditFileA->setText(fileName);
}

void DiffLoadDialog::onButtonFileBOpenClicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select File B"));
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
        QMessageBox::warning(this, tr("Invalid File Path"),
                             tr("Please provide a valid path for File B"));
        return;
    }

    ui->lineEditFileB->setText(fileName);
}

void DiffLoadDialog::onButtonBoxAccepted()
{
    const QFileInfo infoA(ui->lineEditFileA->text());
    if (ui->lineEditFileA->text().isEmpty() || !infoA.exists() || !infoA.isFile()) {
        QMessageBox::warning(this, tr("Invalid File Path"),
                             tr("Please provide a valid path for File A"));
        return;
    }

    const QFileInfo infoB(ui->lineEditFileB->text());
    if (ui->lineEditFileB->text().isEmpty() || !infoB.exists() || !infoB.isFile()) {
        QMessageBox::warning(this, tr("Invalid File Path"),
                             tr("Please provide a valid path for File B"));
        return;
    }

    if (!infoA.isReadable()) {
        QMessageBox::warning(this, tr("Cannot open the file"),
                             tr("The selected File A cannot be read."));
        return;
    }

    if (!infoB.isReadable()) {
        QMessageBox::warning(this, tr("Cannot open the file"),
                             tr("The selected File B cannot be read."));
        return;
    }

    const BinDiffOptions options = { ui->lineEditFileA->text(), ui->lineEditFileB->text(),
                                     ui->comboBoxAnalysis->currentIndex(),
                                     ui->comboBoxCompare->currentIndex() };

    auto waitDialog = new DiffWaitDialog(cutterDiff.get(), options);

    connect(waitDialog, &QDialog::finished, this, [this, waitDialog](int result) {
        if (result == QDialog::Accepted) {
            auto diffWindow = new CutterDiffWindow(std::move(cutterDiff));
            diffWindow->setAttribute(Qt::WA_DeleteOnClose);
            diffWindow->show();
        }
    });

    waitDialog->setAttribute(Qt::WA_DeleteOnClose);
    waitDialog->show(ui->lineEditFileA->text(), ui->lineEditFileB->text());
    accept();
}

void DiffLoadDialog::onButtonBoxRejected() {}
