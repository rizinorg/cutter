#include "CutterDiffWindow.h"

#include "DiffExportDialog.h"
#include "DiffLoadDialog.h"

// Widgets
#include "DiffMatchWidget.h"
#include "DiffMisMatchWidget.h"
#include "FunctionDiffNavWidget.h"
#include "GraphDiffWidget.h"
#include "HexDiffWidget.h"
#include "LineDiffWidget.h"
#include "ui_CutterDiffWindow.h"

#include <QApplication>
#include <QClipboard>

#include <Configuration.h>

CutterDiffWindow::CutterDiffWindow(std::unique_ptr<CutterDiff> cutterDiff, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::CutterDiffWindow),
      cutterDiff(std::move(cutterDiff)),
      functionDiffNavWidget(new FunctionDiffNavWidget(this->cutterDiff.get(), this)),
      matchWidget(new DiffMatchWidget(this->cutterDiff.get(), this)),
      addedWidget(new DiffMisMatchWidget(this->cutterDiff.get(), this, false)),
      removedWidget(new DiffMisMatchWidget(this->cutterDiff.get(), this, true))
{
    ui->setupUi(this);
    ui->splitter->insertWidget(0, functionDiffNavWidget);
    ui->splitter->setSizes({ 250, 750 });
    ui->splitterHexView->setSizes({ 750, 250 });

    ui->tabMatches->layout()->addWidget(matchWidget);
    ui->tabAdded->layout()->addWidget(addedWidget);
    ui->tabRemoved->layout()->addWidget(removedWidget);

    connect(cutterDiff.get(), &CutterDiff::diffDataUpdated, this, &CutterDiffWindow::showDiff);
    connect(ui->actionDiffNewFiles, &QAction::triggered, this,
            &CutterDiffWindow::onActionDiffNewFile);
    connect(ui->actionExportToJSON, &QAction::triggered, this, &CutterDiffWindow::exportDiff);

    connect(cutterDiff.get(), &CutterDiff::diffDataUpdated, this, &CutterDiffWindow::showDiff);
    // ui->tabParsing->hide();

    setupFonts();
    showMaximized();

    // Add widgets
    addHexDiff();
    addLineDiff();
    addGraphDiff();

    showDiff();
}

CutterDiffWindow::~CutterDiffWindow()
{
    delete ui;
}

void CutterDiffWindow::showGraphDiff()
{
    ui->tabWidget->setCurrentWidget(ui->tabGraphDiff);
}

void CutterDiffWindow::showLineDiff()
{
    ui->tabWidget->setCurrentWidget(ui->tabLineDiff);
}

void CutterDiffWindow::showHexDiff()
{
    ui->tabWidget->setCurrentWidget(ui->tabHexDiff);
}

void CutterDiffWindow::showMatches()
{
    ui->tabWidget->setCurrentWidget(ui->tabMatches);
}

void CutterDiffWindow::showRemoved()
{
    ui->tabWidget->setCurrentWidget(ui->tabRemoved);
}

void CutterDiffWindow::showAdded()
{
    ui->tabWidget->setCurrentWidget(ui->tabAdded);
}

void CutterDiffWindow::seekAndShowHexDiff(QPair<RVA, RVA> addr)
{
    showHexDiff();
    hexDiff->seek(addr);
}

void CutterDiffWindow::setupFonts() {}

void CutterDiffWindow::showDiff()
{
    matchWidget->reload();
    addedWidget->reload();
    removedWidget->reload();
}

void CutterDiffWindow::onActionDiffNewFile()
{
    auto loadDiff = new DiffLoadDialog(parentWidget());
    loadDiff->show();
    loadDiff->raise();
}

void CutterDiffWindow::addHexDiff()
{
    if (!hexDiff) {
        hexDiff = new HexDiffWidget(cutterDiff.get(), this);
    }
    ui->hexDiffContainer->addWidget(hexDiff);
}

void CutterDiffWindow::addLineDiff()
{
    if (!lineDiff) {
        lineDiff = new LineDiffWidget(cutterDiff.get(), this);
    }
    ui->lineDiffContainer->layout()->addWidget(lineDiff);
}

void CutterDiffWindow::addGraphDiff()
{
    if (!graphDiff) {
        graphDiff = new GraphDiffWidget(cutterDiff.get(), this);
    }
    ui->graphDiffContainer->addWidget(graphDiff);
}

void CutterDiffWindow::exportDiff()
{
    DiffExportDialog dialog(cutterDiff.get(), this);
    dialog.exec();
}
