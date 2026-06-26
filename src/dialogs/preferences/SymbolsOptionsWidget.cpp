#include "SymbolsOptionsWidget.h"

#include "PreferencesDialog.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "ui_SymbolsOptionsWidget.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QPushButton>
#include <QUrl>

SymbolsOptionsWidget::SymbolsOptionsWidget(PreferencesDialog *parent)
    : QDialog(parent), mainWindow(parent->getMainWindow()), ui(new Ui::SymbolsOptionsWidget)
{
    ui->setupUi(this);

    updateSymbolsOptions();

    auto symbolOptionsChanged = [this] {
        disconnect(Core(), &CutterCore::symbolsOptionsChanged, this,
                   &SymbolsOptionsWidget::updateSymbolsOptions);
        Core()->triggerSymbolsOptionsChanged();
        connect(Core(), &CutterCore::symbolsOptionsChanged, this,
                &SymbolsOptionsWidget::updateSymbolsOptions);
    };

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    connect(ui->debuginfodCheckBox, &QCheckBox::checkStateChanged, this,
            [this, symbolOptionsChanged](bool checked) {
                updateDebuginfodLayout();
                Core()->setConfig("bin.dbginfo.debuginfod", checked);
                symbolOptionsChanged();
            });
#else
    connect(ui->debuginfodCheckBox, &QCheckBox::stateChanged, this,
            [this, symbolOptionsChanged](bool checked) {
                updateDebuginfodLayout();
                Core()->setConfig("bin.dbginfo.debuginfod", checked);
                symbolOptionsChanged();
            });
#endif
    connect(ui->pdbSelect, &QPushButton::clicked, this,
            &SymbolsOptionsWidget::pdbSelectButtonClicked);
    connect(ui->reanalyzeButton, &QPushButton::clicked, this, &SymbolsOptionsWidget::reanalyze);

    connect(ui->debuginfodLineEdit, &QLineEdit::editingFinished, this,
            [this, symbolOptionsChanged] {
                Core()->setConfig("bin.dbginfo.debuginfod_urls", ui->debuginfodLineEdit->text());
                symbolOptionsChanged();
            });
    connect(ui->pdbServerEdit, &QLineEdit::editingFinished, this, [this, symbolOptionsChanged] {
        Core()->setConfig("pdb.server", ui->pdbServerEdit->text());
        symbolOptionsChanged();
    });
    connect(Core(), &CutterCore::symbolsOptionsChanged, this,
            &SymbolsOptionsWidget::updateSymbolsOptions);
}

SymbolsOptionsWidget::~SymbolsOptionsWidget() {}

void SymbolsOptionsWidget::updateSymbolsOptions()
{
    // pdbServer
    ui->pdbServerEdit->setText(Core()->getConfig("pdb.server"));
    // debuginfod
    ui->debuginfodCheckBox->setChecked(Core()->getConfigb("bin.dbginfo.debuginfod"));
    ui->debuginfodLineEdit->setText(Core()->getConfig("bin.dbginfo.debuginfod_urls"));
    updateDebuginfodLayout();
}

void SymbolsOptionsWidget::pdbSelectButtonClicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select PDB file"), QString(),
                                                          tr("PDB file (*.pdb);;All files (*)"));

    if (!fileName.isEmpty()) {
        ui->pdbLineEdit->setText(QDir::toNativeSeparators(fileName));
    }
}

void SymbolsOptionsWidget::updateDebuginfodLayout()
{
    ui->debuginfodLineEdit->setEnabled(ui->debuginfodCheckBox->isChecked());
}

void SymbolsOptionsWidget::reanalyze()
{
    mainWindow->onActionAnalyzeTriggered();
    const QUrl pdbFile = QUrl::fromUserInput(ui->pdbLineEdit->text());
    if (pdbFile.isValid() && pdbFile.isLocalFile()) {
        const QFileInfo pdbFileInfo(pdbFile.toLocalFile());
        if (pdbFileInfo.exists() && pdbFileInfo.isFile()) {
            Core()->loadPDB(ui->pdbLineEdit->text());
            mainWindow->refreshAll();
            Core()->message(tr("%1 loaded.").arg(ui->pdbLineEdit->text()));
        }
    }
}
