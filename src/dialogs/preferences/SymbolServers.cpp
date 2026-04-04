#include "SymbolServers.h"
#include "ui_SymbolServers.h"
#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QUrl>
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "common/AnalysisTask.h"
#include "common/AsyncTask.h"
#include "dialogs/AsyncTaskDialog.h"
#include "common/Configuration.h"
#include "PreferencesDialog.h"
#include "CutterApplication.h"

SymbolServers::SymbolServers(PreferencesDialog *parent)
    : QDialog(parent), mainWindow(parent->getMainWindow()), ui(new Ui::SymbolServers)
{
    ui->setupUi(this);

    // pdbServer
    ui->pdbServerEdit->setText(Core()->getConfig("pdb.server"));
    // debuginfod
    ui->debuginfodCheckBox->setChecked(Core()->getConfigb("bin.dbginfo.debuginfod"));
    ui->debuginfodLineEdit->setText(Core()->getConfig("bin.dbginfo.debuginfod_urls"));
    updateDebuginfodLayout();
    connect(ui->debuginfodCheckBox, &QCheckBox::stateChanged, this,
            &SymbolServers::updateDebuginfodLayout);
    connect(ui->pdbSelect, &QPushButton::clicked, this, &SymbolServers::pdbSelectButtonClicked);
    connect(ui->reanalyzeButton, &QPushButton::clicked, this, &SymbolServers::reanalyze);
}

SymbolServers::~SymbolServers() {}

void SymbolServers::pdbSelectButtonClicked()
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

void SymbolServers::updateDebuginfodLayout()
{
    ui->debuginfodLineEdit->setEnabled(ui->debuginfodCheckBox->isChecked());
}

void SymbolServers::reanalyze()
{
    Core()->setConfig("bin.dbginfo.debuginfod", ui->debuginfodCheckBox->isChecked());
    Core()->setConfig("bin.dbginfo.debuginfod_urls", ui->debuginfodLineEdit->text());
    Core()->setConfig("pdb.server", ui->pdbServerEdit->text());

    mainWindow->on_actionAnalyze_triggered();
    QUrl pdbFile = QUrl::fromUserInput(ui->pdbLineEdit->text());
    if (pdbFile.isValid() && pdbFile.isLocalFile()) {
        QFileInfo pdbFileInfo(pdbFile.toLocalFile());
        if (pdbFileInfo.exists() && pdbFileInfo.isFile()) {
            Core()->loadPDB(ui->pdbLineEdit->text());
            mainWindow->refreshAll();
            Core()->message(tr("%1 loaded.").arg(ui->pdbLineEdit->text()));
        }
    }
}
