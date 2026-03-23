#include "SymbolServers.h"
#include "ui_SymbolServers.h"
#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QUrl>
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "common/Configuration.h"

SymbolServers::SymbolServers(QWidget *parent) : QDialog(parent), ui(new Ui::SymbolServers)
{
    ui->setupUi(this);
    // debuginfod
    ui->debuginfodCheckBox->setChecked(Core()->getConfig("bin.dbginfo.debuginfod") == "true");
    ui->debuginfodLineEdit->setText(Core()->getConfig("bin.dbginfo.debuginfod_urls"));
    updateDebuginfodLayout();
    connect(ui->debuginfodCheckBox, &QCheckBox::stateChanged, this,
            &SymbolServers::updateDebuginfodLayout);
    updatePDBLayout();
    connect(ui->pdbCheckBox, &QCheckBox::stateChanged, this, &SymbolServers::updatePDBLayout);
    connect(ui->pdbSelect, &QPushButton::clicked, this, &SymbolServers::on_pdbSelectButton_clicked);
    connect(ui->reanalyzeButton, &QPushButton::clicked, this, &SymbolServers::reanalyze);
}

void SymbolServers::reanalyze()
{
    QUrl pdbFile = QUrl::fromUserInput(ui->pdbLineEdit->text());
    if (pdbFile.isValid() && pdbFile.isLocalFile()) {
        QFileInfo pdbFileInfo(pdbFile.toLocalFile());
        if (pdbFileInfo.exists() && pdbFileInfo.isFile())
            Core()->loadPDB(ui->pdbLineEdit->text());
    }
    Core()->setConfig("bin.dbginfo.debuginfod", ui->debuginfodCheckBox->isChecked());
    Core()->setConfig("bin.dbginfo.debuginfod_urls", ui->debuginfodLineEdit->text());
    Core()->applyDwarf();
    auto mainWindow = new MainWindow(this);
    mainWindow->on_actionAnalyze_triggered();
}

SymbolServers::~SymbolServers() {}

void SymbolServers::updateDebuginfodLayout()
{
    ui->debuginfodLineEdit->setEnabled(ui->debuginfodCheckBox->isChecked());
}

void SymbolServers::updatePDBLayout()
{
    ui->pdbWidget->setEnabled(ui->pdbCheckBox->isChecked());
}

void SymbolServers::on_pdbSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select PDB file"));
    dialog.setNameFilters({ tr("PDB file (*.pdb)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (!fileName.isEmpty()) {
        ui->pdbLineEdit->setText(fileName);
    }
}
