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
#include <QTabWidget>

#include <Configuration.h>
#include <shortcuts/ShortcutManager.h>

CutterDiffWindow::CutterDiffWindow(std::unique_ptr<CutterDiff> cutterDiff, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::CutterDiffWindow),
      cutterDiff(std::move(cutterDiff)),
      functionDiffNavWidget(new FunctionDiffNavWidget(this->cutterDiff.get(), this)),
      matchWidget(new DiffMatchWidget(this->cutterDiff.get(), this)),
      addedWidget(new DiffMisMatchWidget(this->cutterDiff.get(), this, false)),
      removedWidget(new DiffMisMatchWidget(this->cutterDiff.get(), this, true))
{
    const QSettings settings;
    ui->setupUi(this);
    ui->splitter->insertWidget(0, functionDiffNavWidget);
    ui->splitter->setSizes({ 250, 750 });
    ui->splitterHexView->setSizes({ 750, 250 });
    centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    statusBar()->hide();

    ui->tabMatches->layout()->addWidget(matchWidget);
    ui->tabAdded->layout()->addWidget(addedWidget);
    ui->tabRemoved->layout()->addWidget(removedWidget);
    connect(this->cutterDiff.get(), &CutterDiff::diffDataUpdated, this,
            &CutterDiffWindow::showDiff);
    connect(ui->actionDiffNewFiles, &QAction::triggered, this,
            &CutterDiffWindow::onActionDiffNewFile);
    connect(ui->actionExportToJSON, &QAction::triggered, this, &CutterDiffWindow::exportDiff);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &CutterDiffWindow::onTabIndexChanged);
    connect(this->cutterDiff.get(), &CutterDiff::currentItemDiffChanged, this,
            &CutterDiffWindow::addSeekHistory);

    Shortcuts()->setupAction(*ui->actionUndoSeek, "General.back");
    Shortcuts()->setupAction(*ui->actionRedoSeek, "General.forward");

    connect(ui->actionUndoSeek, &QAction::triggered, this, &CutterDiffWindow::seekHistoryBackward);
    connect(ui->actionRedoSeek, &QAction::triggered, this, &CutterDiffWindow::seekHistoryForward);

    // ui->tabParsing->hide();
    setupFonts();
    showMaximized();

    // Add widgets
    addHexDiff();
    addLineDiff();
    addGraphDiff();

    showDiff();
    showHexDiff();

    restoreGeometry(settings.value("DiffWindow/geometry").toByteArray());
    restoreState(settings.value("DiffWindow/state").toByteArray(), 1);
}

CutterDiffWindow::~CutterDiffWindow()
{
    delete ui;
}

void CutterDiffWindow::showPrevMemoryWidget()
{
    ui->tabWidget->setCurrentIndex(lastMemoryWidget);
}

void CutterDiffWindow::onTabIndexChanged()
{
    addSeekHistory();
    if (!(ui->tabWidget->currentIndex() < HexDiff)) {
        lastMemoryWidget = ui->tabWidget->currentIndex();
    }
}

void CutterDiffWindow::seekHistoryBackward()
{
    if (seekHistory.empty() || currentSeekIndex == 0) {
        return;
    }

    --currentSeekIndex;
    seekToCurrentHistory();
    updateSeekActions();
}

void CutterDiffWindow::seekHistoryForward()
{
    if (seekHistory.empty() || currentSeekIndex + 1 >= seekHistory.size()) {
        return;
    }
    ++currentSeekIndex;
    seekToCurrentHistory();
    updateSeekActions();
}

void CutterDiffWindow::seekToCurrentHistory()
{
    if (seekHistory.empty() || currentSeekIndex >= seekHistory.size()) {
        return;
    }

    const auto &currentHistory = seekHistory[currentSeekIndex];

    seekingHistory = true;

    cutterDiff->setCurrentDiffItemIndex(currentHistory.diffItemIndex);

    ui->tabWidget->setCurrentIndex(currentHistory.tabIndex);

    seekingHistory = false;
}

void CutterDiffWindow::addSeekHistory()
{
    if (seekingHistory || !cutterDiff || ui->tabWidget->currentIndex() < HexDiff) {
        return;
    }

    const DiffSeekLocation location { ui->tabWidget->currentIndex(),
                                      cutterDiff->getCurrentDiffItemIndex() };

    // Don't add duplicate consecutive locations.
    if (!seekHistory.empty() && seekHistory.back() == location) {
        return;
    }

    // Remove forward history.
    if (!seekHistory.empty() && currentSeekIndex + 1 < seekHistory.size()) {

        seekHistory.erase(seekHistory.begin() + currentSeekIndex + 1, seekHistory.end());
    }

    seekHistory.push_back(location);
    currentSeekIndex = seekHistory.size() - 1;

    // Limit history size.
    if (seekHistory.size() > maxSeekHistory) {
        seekHistory.pop_front();
        --currentSeekIndex;
    }

    updateSeekActions();
}

void CutterDiffWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;

    settings.setValue("DiffWindow/geometry", saveGeometry());
    settings.setValue("DiffWindow/state", saveState(1));

    QMainWindow::closeEvent(event);
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
    seekHistory.clear();
    currentSeekIndex = 0;

    seekHistory.push_back({ HexDiff, cutterDiff->getCurrentDiffItemIndex() });

    emit reload();

    updateSeekActions();
}

void CutterDiffWindow::updateSeekActions()
{
    ui->actionUndoSeek->setEnabled(!seekHistory.empty() && currentSeekIndex > 0);

    ui->actionRedoSeek->setEnabled(!seekHistory.empty()
                                   && currentSeekIndex + 1 < seekHistory.size());
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

void CutterDiffWindow::showDiffItemContextMenu(const QPoint &pos, int diffItemIndex, bool orig)
{
    const CutterDiffItem &diffItem = cutterDiff->getDiffItemAt(diffItemIndex);

    if (!diffItem.isFunction()) {
        return;
    }
    QMenu menu(this);

    const QAction *seekTo = menu.addAction("Seek to");
    const QAction *diffFunctionLines = menu.addAction("Line Diff");
    const QAction *copyAddress = menu.addAction("Copy Address");
    const QAction *showGraphDiff = menu.addAction("Graph Diff");

    const QAction *selected = menu.exec(pos);

    if (diffItem.getType() == DiffItemMatched) {
        if (selected == seekTo) {
            if (orig) {
                seekAndShowHexDiff({ diffItem.functionA().offset, RVA_INVALID });
            } else {
                seekAndShowHexDiff({ RVA_INVALID, diffItem.functionB().offset });
            }
        } else if (selected == copyAddress) {
            if (orig) {
                QApplication::clipboard()->setText(rzAddressString(diffItem.functionA().offset));
            } else {
                QApplication::clipboard()->setText(rzAddressString(diffItem.functionB().offset));
            }
        }
    } else {
        if (selected == seekTo) {
            if (diffItem.getType() == DiffItemRemoved) {
                seekAndShowHexDiff({ diffItem.functionA().offset, RVA_INVALID });
            } else {
                seekAndShowHexDiff({ RVA_INVALID, diffItem.functionB().offset });
            }
        } else if (selected == copyAddress) {
            if (diffItem.getType() == DiffItemRemoved) {
                QApplication::clipboard()->setText(rzAddressString(diffItem.functionA().offset));
            } else {
                QApplication::clipboard()->setText(rzAddressString(diffItem.functionB().offset));
            }
        }
    }
    if (selected == diffFunctionLines) {
        cutterDiff->setCurrentDiffItemIndex(diffItemIndex);
        this->showLineDiff();
    } else if (selected == showGraphDiff) {
        cutterDiff->setCurrentDiffItemIndex(diffItemIndex);
        this->showGraphDiff();
    }
}

CutterDiffWidget::CutterDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
    : QWidget(parent), cutterDiff(cutterDiff), diffWindow(parent)
{
    connect(parent, &CutterDiffWindow::reload, this, &CutterDiffWidget::reload);
}