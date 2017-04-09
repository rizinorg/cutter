#include "xrefsdialog.h"
#include "ui_xrefsdialog.h"

#include "mainwindow.h"

XrefsDialog::XrefsDialog(MainWindow *main, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::XrefsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    this->main = main;

    // Increase asm text edit margin
    QTextDocument *asm_docu = ui->previewTextEdit->document();
    asm_docu->setDocumentMargin(10);

    // Syntax highlight
    highlighter = new Highlighter(this->main, ui->previewTextEdit->document());

    // Highlight current line
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
}

XrefsDialog::~XrefsDialog()
{
    delete ui;
}

void XrefsDialog::fillRefs(QList<QStringList> refs, QList<QStringList> xrefs) {
    ui->fromTreeWidget->clear();
    for (int i = 0; i < refs.size(); ++i) {
        //this->add_debug_output(refs.at(i).at(0) + " " + refs.at(i).at(1));
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, refs.at(i).at(0));
        tempItem->setText(1, refs.at(i).at(1));
        //tempItem->setToolTip( 0, this->main->core->cmd("pdi 10 @ " + refs.at(i).at(0)) );
        //tempItem->setToolTip( 1, this->main->core->cmd("pdi 10 @ " + refs.at(i).at(0)) );
        ui->fromTreeWidget->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count = ui->fromTreeWidget->columnCount();
    for (int i = 0; i != count; ++i) {
        ui->fromTreeWidget->resizeColumnToContents(i);
    }

    ui->toTreeWidget->clear();
    for (int i = 0; i < xrefs.size(); ++i) {
        //this->add_debug_output(xrefs.at(i).at(0) + " " + xrefs.at(i).at(1));
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, xrefs.at(i).at(0));
        tempItem->setText(1, xrefs.at(i).at(1));
        //tempItem->setToolTip( 0, this->main->core->cmd("pdi 10 @ " + xrefs.at(i).at(0)) );
        //tempItem->setToolTip( 1, this->main->core->cmd("pdi 10 @ " + xrefs.at(i).at(0)) );
        ui->toTreeWidget->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count2 = ui->toTreeWidget->columnCount();
    for (int i = 0; i != count2; ++i) {
        ui->toTreeWidget->resizeColumnToContents(i);
    }

}

void XrefsDialog::on_fromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    QString offset = item->text(0);
    RAnalFunction *fcn = this->main->core->functionAt(offset.toLongLong(0, 16));
    //this->add_debug_output( fcn->name );
    this->main->seek(offset, fcn->name);
    this->close();
}

void XrefsDialog::on_toTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    QString offset = item->text(0);
    RAnalFunction *fcn = this->main->core->functionAt(offset.toLongLong(0, 16));
    //this->add_debug_output( fcn->name );
    this->main->seek(offset, fcn->name);
    this->close();
}

QString XrefsDialog::normalizeAddr(QString addr) {
    QString base = addr.split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8) {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        QString s = "0x" + zeroes + base;
        return s;
    } else {
        return addr;
    }
}

void XrefsDialog::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->previewTextEdit->isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->previewTextEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);

        ui->previewTextEdit->setExtraSelections(extraSelections);
    }
}

void XrefsDialog::on_fromTreeWidget_itemSelectionChanged()
{
    QTreeWidgetItem *item = ui->fromTreeWidget->currentItem();
    QString offset = item->text(0);
    ui->previewTextEdit->setPlainText( this->main->core->cmd("pdf @ " + offset).trimmed() );
    ui->previewTextEdit->moveCursor(QTextCursor::End);
    // Does it make any sense?
    ui->previewTextEdit->find( this->normalizeAddr(offset), QTextDocument::FindBackward);
    ui->previewTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
}

void XrefsDialog::on_toTreeWidget_itemSelectionChanged()
{
    QTreeWidgetItem *item = ui->toTreeWidget->currentItem();
    QString offset = item->text(0);
    ui->previewTextEdit->setPlainText( this->main->core->cmd("pdf @ " + offset).trimmed() );
    ui->previewTextEdit->moveCursor(QTextCursor::End);
    // Again, does it make any sense?
    // Also, this code should be refactored and shared instead of copied & pasted
    ui->previewTextEdit->find( this->normalizeAddr(offset), QTextDocument::FindBackward);
    ui->previewTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
}

void XrefsDialog::updateLabels(QString name) {
    ui->label_2->setText(ui->label_2->text() + name);
    ui->label_3->setText(ui->label_3->text() + name);
}
