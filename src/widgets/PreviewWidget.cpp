#include "PreviewWidget.h"
#include "ui_PreviewWidget.h"
#include "DisassemblerGraphView.h"

#include "utils/Helpers.h"

#include <QClipboard>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QSettings>

PreviewWidget::PreviewWidget(QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(parent, flags),
    ui(new Ui::PreviewWidget),
    core(CutterCore::getInstance())
{
    ui->setupUi(this);

    // Highlight current line on previews and decompiler
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightPreviewCurrentLine()));
    connect(ui->decoTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightDecoCurrentLine()));

    // Hide tabs
    QTabBar *preTab = ui->memPreviewTab->tabBar();
    preTab->setVisible(false);

    connect(core, SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
}

PreviewWidget::PreviewWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags) :
    PreviewWidget(parent, flags)
{
    setWindowTitle(title);
}

void PreviewWidget::on_seekChanged(RVA addr)
{
    refresh(addr);
}

void PreviewWidget::highlightPreviewCurrentLine()
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

void PreviewWidget::highlightDecoCurrentLine()
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

PreviewWidget::~PreviewWidget() {}

void PreviewWidget::refresh(RVA addr)
{
    if (addr == RVA_INVALID)
    {
        addr = core->getSeekAddr();
    }

    setMiniGraph(RAddressString(addr));

    // TODO: pseudo, ...
}

/*
 * Actions callback functions
 */

void PreviewWidget::on_actionSettings_menu_1_triggered()
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

void PreviewWidget::setFonts(QFont font)
{
    ui->previewTextEdit->setFont(font);
    ui->decoTextEdit->setFont(font);
}

/*
 * Buttons callback functions
 */


void PreviewWidget::setMiniGraph(QString at)
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

void PreviewWidget::on_previewToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(0);
}

void PreviewWidget::on_decoToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(1);
}

void PreviewWidget::on_simpleGrapgToolButton_clicked()
{
    ui->memPreviewTab->setCurrentIndex(2);
}

void PreviewWidget::switchTheme(bool dark)
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