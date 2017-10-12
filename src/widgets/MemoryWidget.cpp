#include "MemoryWidget.h"
#include "ui_MemoryWidget.h"
#include "DisassemblerGraphView.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTemporaryFile>
#include <QFontDialog>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>
#include <QWebEnginePage>
#include <QMenu>
#include <QFont>
#include <QUrl>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QSettings>

#include <cassert>

MemoryWidget::MemoryWidget() :
    ui(new Ui::MemoryWidget),
    core(CutterCore::getInstance())
{
    ui->setupUi(this);
    this->memTabWidget = ui->memTabWidget;

    //this->on_actionSettings_menu_1_triggered();

    // Setup hex highlight
    //connect(ui->hexHexText, SIGNAL(cursorPositionChanged()), this, SLOT(highlightHexCurrentLine()));
    //highlightHexCurrentLine();

    // Highlight current line on previews and decompiler
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightPreviewCurrentLine()));
    connect(ui->decoTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightDecoCurrentLine()));

    // Hide memview notebooks tabs
    QTabBar *bar = ui->memTabWidget->tabBar();
    bar->setVisible(false);
    QTabBar *sidebar = ui->memSideTabWidget_2->tabBar();
    sidebar->setVisible(false);
    QTabBar *preTab = ui->memPreviewTab->tabBar();
    preTab->setVisible(false);

    // Debug console
    // For QWebEngine debugging see: https://doc.qt.io/qt-5/qtwebengine-debugging.html
    //QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    // Popup menu on Settings toolbutton
    QMenu *memMenu = new QMenu();
    ui->memSettingsButton_2->addAction(ui->actionSettings_menu_1);
    memMenu->addAction(ui->actionSettings_menu_1);
    ui->memSettingsButton_2->setMenu(memMenu);

    // Set Splitter stretch factor
    ui->splitter->setStretchFactor(0, 10);
    ui->splitter->setStretchFactor(1, 1);

    // Space to switch between disassembly and graph
    QShortcut *graph_shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(graph_shortcut, SIGNAL(activated()), this, SLOT(cycleViews()));
    //graph_shortcut->setContext(Qt::WidgetShortcut);

    connect(core, SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(core, SIGNAL(flagsChanged()), this, SLOT(updateViews()));
    connect(core, SIGNAL(commentsChanged()), this, SLOT(updateViews()));
    connect(core, SIGNAL(asmOptionsChanged()), this, SLOT(updateViews()));
}


void MemoryWidget::on_seekChanged(RVA addr)
{
    updateViews(addr);
}

void MemoryWidget::highlightPreviewCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->previewTextEdit->toPlainText() != "")
    {
        if (ui->previewTextEdit->isReadOnly())
        {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(190, 144, 212);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = ui->previewTextEdit->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }
    ui->previewTextEdit->setExtraSelections(extraSelections);
}

void MemoryWidget::highlightDecoCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->decoTextEdit->toPlainText() != "")
    {
        if (ui->decoTextEdit->isReadOnly())
        {
            QTextEdit::ExtraSelection selection;

            QColor lineColor = QColor(190, 144, 212);

            selection.format.setBackground(lineColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = ui->decoTextEdit->textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }
    ui->decoTextEdit->setExtraSelections(extraSelections);
}

MemoryWidget::~MemoryWidget() {}

void MemoryWidget::setup()
{
    const QString off = core->cmd("afo entry0").trimmed();
    RVA offset = off.toULongLong(0, 16);
    updateViews(offset);

    //refreshDisasm();
    //refreshHexdump(off);
    //create_graph(off);
    //setFcnName(off);
}

void MemoryWidget::refresh()
{
    // TODO: honor the offset
    updateViews(RVA_INVALID);
}

/*
 * Show widgets
 */

void MemoryWidget::cycleViews()
{
    switch (ui->memTabWidget->currentIndex())
    {
    case 1:
        // Show disasm
        ui->disasButton->setChecked(true);
        on_disasButton_clicked();
        break;
    }
}

/*
 * Actions callback functions
 */

void MemoryWidget::on_actionSettings_menu_1_triggered()
{
    bool ok = true;

    QFont font = QFont("Monospace", 8);
    // TODO Use global configuration
    //QFont font = QFontDialog::getFont(&ok, ui->disasTextEdit_2->font(), this);

    if (ok)
    {
        setFonts(font);

        emit fontChanged(font);
    }
}

void MemoryWidget::setFonts(QFont font)
{
    ui->previewTextEdit->setFont(font);
    ui->decoTextEdit->setFont(font);
}

void MemoryWidget::on_actionHideDisasm_side_panel_triggered()
{
    if (ui->memSideTabWidget_2->isVisible())
    {
        ui->memSideTabWidget_2->hide();
    }
    else
    {
        ui->memSideTabWidget_2->show();
    }
}


void MemoryWidget::on_actionHideGraph_side_panel_triggered()
{
    if (ui->graphTreeWidget_2->isVisible())
    {
        ui->graphTreeWidget_2->hide();
    }
    else
    {
        ui->graphTreeWidget_2->show();
    }
}

/*
 * Buttons callback functions
 */

void MemoryWidget::on_disasButton_clicked()
{
    ui->memTabWidget->setCurrentIndex(0);
    ui->memSideTabWidget_2->setCurrentIndex(0);
}

void MemoryWidget::on_hexButton_clicked()
{
    ui->memTabWidget->setCurrentIndex(1);
    ui->memSideTabWidget_2->setCurrentIndex(1);
}


void MemoryWidget::setMiniGraph(QString at)
{
    QString dot = this->core->getSimpleGraph(at);
    //QString dot = this->core->cmd("agc " + at);
    // Add data to HTML Polar functions graph
    QFile html(":/html/graph.html");
    if (!html.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this, "error", html.errorString());
    }
    QString code = html.readAll();
    html.close();

    code.replace("MEOW", dot);
    ui->webSimpleGraph->setHtml(code);

}


void MemoryWidget::on_hexSideTab_2_currentChanged(int /*index*/)
{
    /*
    if (index == 2) {
        // Add data to HTML Polar functions graph
        QFile html(":/html/bar.html");
        if(!html.open(QIODevice::ReadOnly)) {
            QMessageBox::information(0,"error",html.errorString());
        }
        QString code = html.readAll();
        html.close();
        this->histoWebView->setHtml(code);
        this->histoWebView->show();
    } else {
        this->histoWebView->hide();
    }
    */
}

void MemoryWidget::on_memSideToolButton_clicked()
{
    if (ui->memSideToolButton->isChecked())
    {
        ui->memSideTabWidget_2->hide();
        ui->memSideToolButton->setIcon(QIcon(":/img/icons/left_light.svg"));
    }
    else
    {
        ui->memSideTabWidget_2->show();
        ui->memSideToolButton->setIcon(QIcon(":/img/icons/right_light.svg"));
    }
}

void MemoryWidget::on_previewToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(0);
}

void MemoryWidget::on_decoToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(1);
}

void MemoryWidget::on_simpleGrapgToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(2);
}

void MemoryWidget::on_previewToolButton_2_clicked()
{
    if (ui->previewToolButton_2->isChecked())
    {
        ui->frame_3->setVisible(true);
    }
    else
    {
        ui->frame_3->setVisible(false);
    }
}

void MemoryWidget::resizeEvent(QResizeEvent *event)
{
    // FIXME
    /*
    if (main->responsive && isVisible())
    {
        if (event->size().width() <= 1150)
        {
            ui->frame_3->setVisible(false);
            ui->memPreviewTab->setVisible(false);
            ui->previewToolButton_2->setChecked(false);
            if (event->size().width() <= 950)
            {
                ui->memSideTabWidget_2->hide();
                ui->hexSideTab_2->hide();
                ui->memSideToolButton->setChecked(true);
            }
            else
            {
                ui->memSideTabWidget_2->show();
                ui->hexSideTab_2->show();
                ui->memSideToolButton->setChecked(false);
            }
        }
        else
        {
            ui->frame_3->setVisible(true);
            ui->memPreviewTab->setVisible(true);
            ui->previewToolButton_2->setChecked(true);
        }
    }
    */
    QDockWidget::resizeEvent(event);
}

void MemoryWidget::switchTheme(bool dark)
{
    if (dark)
    {
        ui->webSimpleGraph->page()->setBackgroundColor(QColor(64, 64, 64));
    }
    else
    {
        ui->webSimpleGraph->page()->setBackgroundColor(QColor(255, 255, 255));
    }
}

void MemoryWidget::seek_back()
{
    //this->main->add_debug_output("Back!");
    // FIXME
    // this->main->backButton_clicked();
}

void MemoryWidget::on_memTabWidget_currentChanged(int /*index*/)
{
    /*this->main->add_debug_output("Update index: " + QString::number(index) + " to function: " + RAddressString(main->getCursorAddress()));
    this->main->add_debug_output("Last disasm: " + RAddressString(this->last_disasm_fcn));
    this->main->add_debug_output("Last graph: " + RAddressString(this->last_graph_fcn));
    this->main->add_debug_output("Last hexdump: " + RAddressString(this->last_hexdump_fcn));*/
    this->updateViews(RVA_INVALID);
}

void MemoryWidget::updateViews(RVA offset)
{
    // Update only the selected view to improve performance

    int index = ui->memTabWidget->tabBar()->currentIndex();

    // Anyway updateViews will die after break this widget.
    // FIXME? One cursor per widget ? (if not synced)
    // RVA cursor_addr = main->getCursorAddress();
    // QString cursor_addr_string = RAddressString(cursor_addr);
    QString cursor_addr_string = core->cmd("s");
    RVA cursor_addr = cursor_addr_string.toULongLong();

    // TODO WTF
}