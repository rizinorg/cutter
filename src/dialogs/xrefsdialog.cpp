#include "xrefsdialog.h"
#include "ui_xrefsdialog.h"

#include "mainwindow.h"

#include <QJsonArray>

XrefsDialog::XrefsDialog(MainWindow *main, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::XrefsDialog)
{
    addr = 0;
    func_name = QString::null;

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

void XrefsDialog::fillRefs(QList<XrefDescription> refs, QList<XrefDescription> xrefs)
{
    ui->fromTreeWidget->clear();
    for (int i = 0; i < refs.size(); ++i)
    {
        XrefDescription xref = refs[i];

        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(xref.to));
        tempItem->setText(1, main->core->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        //tempItem->setToolTip( 0, this->main->core->cmd("pdi 10 @ " + refs.at(i).at(0)) );
        //tempItem->setToolTip( 1, this->main->core->cmd("pdi 10 @ " + refs.at(i).at(0)) );

        ui->fromTreeWidget->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count = ui->fromTreeWidget->columnCount();
    for (int i = 0; i != count; ++i)
    {
        ui->fromTreeWidget->resizeColumnToContents(i);
    }

    ui->toTreeWidget->clear();
    for (int i = 0; i < xrefs.size(); ++i)
    {
        XrefDescription xref = xrefs[i];

        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(xref.from));
        tempItem->setText(1, main->core->disassembleSingleInstruction(xref.from));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        //tempItem->setToolTip( 0, this->main->core->cmd("pdi 10 @ " + xrefs.at(i).at(0)) );
        //tempItem->setToolTip( 1, this->main->core->cmd("pdi 10 @ " + xrefs.at(i).at(0)) );

        ui->toTreeWidget->insertTopLevelItem(0, tempItem);
    }
    // Adjust columns to content
    int count2 = ui->toTreeWidget->columnCount();
    for (int i = 0; i != count2; ++i)
    {
        ui->toTreeWidget->resizeColumnToContents(i);
    }

}

void XrefsDialog::on_fromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    RAnalFunction *fcn = this->main->core->functionAt(xref.to);
    this->main->seek(xref.to, fcn ? QString::fromUtf8(fcn->name) : QString::null, true);

    this->close();
}

void XrefsDialog::on_toTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    RAnalFunction *fcn = this->main->core->functionAt(xref.from);
    this->main->seek(xref.from, fcn ? QString::fromUtf8(fcn->name) : QString::null, true);

    this->close();
}

QString XrefsDialog::normalizeAddr(QString addr)
{
    QString base = addr.split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8)
    {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        QString s = "0x" + zeroes + base;
        return s;
    }
    else
    {
        return addr;
    }
}

void XrefsDialog::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->previewTextEdit->isReadOnly())
    {
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
    if (ui->fromTreeWidget->selectedItems().isEmpty())
        return;
    ui->toTreeWidget->clearSelection();
    QTreeWidgetItem *item = ui->fromTreeWidget->currentItem();
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    updatePreview(xref.to);
}

void XrefsDialog::on_toTreeWidget_itemSelectionChanged()
{
    if (ui->toTreeWidget->selectedItems().isEmpty())
        return;
    ui->fromTreeWidget->clearSelection();
    QTreeWidgetItem *item = ui->toTreeWidget->currentItem();
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    updatePreview(xref.from);
}

void XrefsDialog::updatePreview(RVA addr)
{
    QString disass;

    // is the address part of a function, so we can use pdf?
    if (!main->core->cmdj("afij@" + QString::number(addr)).array().isEmpty())
        disass = main->core->cmd("pdf @ " + QString::number(addr));
    else
        disass = main->core->cmd("pd 10 @ " + QString::number(addr));

    ui->previewTextEdit->setPlainText(disass.trimmed());

    // Does it make any sense?
    ui->previewTextEdit->moveCursor(QTextCursor::End);
    ui->previewTextEdit->find(this->normalizeAddr(RAddressString(addr)), QTextDocument::FindBackward);
    ui->previewTextEdit->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
}

void XrefsDialog::updateLabels(QString name)
{
    ui->label_2->setText(tr("X-Refs to %1:").arg(name));
    ui->label_3->setText(tr("X-Refs from %1:").arg(name));
}

void XrefsDialog::fillRefsForFunction(RVA addr, QString name)
{
    this->addr = addr;
    this->func_name = func_name;

    setWindowTitle(tr("X-Refs for function %1").arg(name));
    updateLabels(name);
    // Get Refs and Xrefs

    // refs = calls q hace esa funcion
    QList<XrefDescription> refs = main->core->getXRefs(addr, false, "C");

    // xrefs = calls a esa funcion
    QList<XrefDescription> xrefs = main->core->getXRefs(addr, true);

    fillRefs(refs, xrefs);
}