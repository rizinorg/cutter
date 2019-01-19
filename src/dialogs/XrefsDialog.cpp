#include "XrefsDialog.h"
#include "ui_XrefsDialog.h"

#include "common/TempConfig.h"
#include "common/Helpers.h"

#include "MainWindow.h"

#include <QJsonArray>

XrefsDialog::XrefsDialog(QWidget *parent) :
    QDialog(parent),
    addr(0),
    func_name(QString::null),
    ui(new Ui::XrefsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Modify the splitter's location to show more Disassembly instead of empty space. Not possible via Designer
    ui->splitter->setSizes(QList<int>() << 100 << 200);

    // Increase asm text edit margin
    QTextDocument *asm_docu = ui->previewTextEdit->document();
    asm_docu->setDocumentMargin(10);

    setupPreviewFont();
    setupPreviewColors();

    // Highlight current line
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(setupPreviewFont()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(setupPreviewColors()));
}

XrefsDialog::~XrefsDialog() { }

void XrefsDialog::fillRefs(QList<XrefDescription> refs, QList<XrefDescription> xrefs)
{
    // Fill refs
    ui->fromTreeWidget->clear();
    for (const auto &xref : refs) {
        auto *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, xref.to_str);
        tempItem->setText(1, Core()->disassembleSingleInstruction(xref.to));
        tempItem->setText(2, xrefTypeString(xref.type));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        ui->fromTreeWidget->insertTopLevelItem(0, tempItem);
    }

    // Adjust columns to content
    qhelpers::adjustColumns(ui->fromTreeWidget, 0);

    // Fill Xrefs
    ui->toTreeWidget->clear();
    for (const auto &xref : xrefs) {
        auto *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, xref.from_str);
        tempItem->setText(1, Core()->disassembleSingleInstruction(xref.from));
        tempItem->setText(2, xrefTypeString(xref.type));
        tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
        ui->toTreeWidget->insertTopLevelItem(0, tempItem);
    }

    // Adjust columns to content
    qhelpers::adjustColumns(ui->toTreeWidget, 0);
}

void XrefsDialog::on_fromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    Core()->seek(xref.to);
    this->close();
}

void XrefsDialog::on_toTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    Core()->seek(xref.from);
    this->close();
}

QString XrefsDialog::normalizeAddr(const QString &addr) const
{
    QString r = addr;
    QString base = addr.split("0x")[1].trimmed();
    int len = base.length();
    if (len < 8) {
        int padding = 8 - len;
        QString zero = "0";
        QString zeroes = zero.repeated(padding);
        r = "0x" + zeroes + base;
    }

    return r;
}

void XrefsDialog::setupPreviewFont()
{
    ui->previewTextEdit->setFont(Config()->getFont());
}

void XrefsDialog::setupPreviewColors()
{
    ui->previewTextEdit->setStyleSheet(QString("QPlainTextEdit { background-color: %1; color: %2; }")
                                       .arg(ConfigColor("gui.background").name())
                                       .arg(ConfigColor("btext").name()));
}

void XrefsDialog::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (ui->previewTextEdit->isReadOnly()) {
        QTextEdit::ExtraSelection selection = QTextEdit::ExtraSelection();

        selection.format.setBackground(ConfigColor("highlight"));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->previewTextEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);

        ui->previewTextEdit->setExtraSelections(extraSelections);
    }
}

void XrefsDialog::on_fromTreeWidget_itemSelectionChanged()
{
    if (ui->fromTreeWidget->selectedItems().isEmpty()) {
        return;
    }
    ui->toTreeWidget->clearSelection();
    QTreeWidgetItem *item = ui->fromTreeWidget->currentItem();
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    updatePreview(xref.to);
}

void XrefsDialog::on_toTreeWidget_itemSelectionChanged()
{
    if (ui->toTreeWidget->selectedItems().isEmpty()) {
        return;
    }
    ui->fromTreeWidget->clearSelection();
    QTreeWidgetItem *item = ui->toTreeWidget->currentItem();
    XrefDescription xref = item->data(0, Qt::UserRole).value<XrefDescription>();
    updatePreview(xref.from);
}

void XrefsDialog::updatePreview(RVA addr)
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", true);
    tempConfig.set("scr.color", COLOR_MODE_16M);

    QString disas = Core()->cmd("pd--20 @ " + QString::number(addr));
    ui->previewTextEdit->document()->setHtml(disas);

    // Does it make any sense?
    ui->previewTextEdit->find(normalizeAddr(RAddressString(addr)), QTextDocument::FindBackward);
    ui->previewTextEdit->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
}

void XrefsDialog::updateLabels(QString name)
{
    ui->label_xTo->setText(tr("X-Refs to %1:").arg(name));
    ui->label_xFrom->setText(tr("X-Refs from %1:").arg(name));
}

void XrefsDialog::fillRefsForAddress(RVA addr, QString name, bool whole_function)
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", false);
    tempConfig.set("scr.color", COLOR_MODE_DISABLED);

    setWindowTitle(tr("X-Refs for %1").arg(name));
    updateLabels(name);

    // Get Refs and Xrefs
    QList<XrefDescription> refs = Core()->getXRefs(addr, false, whole_function);
    QList<XrefDescription> xrefs = Core()->getXRefs(addr, true, whole_function);

    fillRefs(refs, xrefs);
}

QString XrefsDialog::xrefTypeString(const QString &type)
{
    switch (type.toStdString()[0]) {
    case R_ANAL_REF_TYPE_CALL:
        return QString("Call");
    case R_ANAL_REF_TYPE_CODE:
        return QString("Code");
    case R_ANAL_REF_TYPE_DATA:
        return QString("Data");
    case R_ANAL_REF_TYPE_NULL:
        return QString("");
    case R_ANAL_REF_TYPE_STRING:
        return QString("String");
    default:
        break;
    }
    return type;
}

