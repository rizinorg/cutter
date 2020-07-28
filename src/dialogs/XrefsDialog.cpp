#include "XrefsDialog.h"
#include "ui_XrefsDialog.h"

#include "common/TempConfig.h"
#include "common/Helpers.h"

#include "core/MainWindow.h"

#include <QJsonArray>

XrefsDialog::XrefsDialog(MainWindow *main, QWidget *parent) :
    QDialog(parent),
    addr(0),
    toModel(this),
    fromModel(this),
    ui(new Ui::XrefsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    ui->toTreeWidget->setMainWindow(main);
    ui->fromTreeWidget->setMainWindow(main);

    ui->toTreeWidget->setModel(&toModel);
    ui->fromTreeWidget->setModel(&fromModel);

    // Modify the splitter's location to show more Disassembly instead of empty space. Not possible via Designer
    ui->splitter->setSizes(QList<int>() << 300 << 400);

    // Increase asm text edit margin
    QTextDocument *asm_docu = ui->previewTextEdit->document();
    asm_docu->setDocumentMargin(10);

    setupPreviewColors();
    setupPreviewFont();

    // Highlight current line
    connect(ui->previewTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(setupPreviewFont()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(setupPreviewColors()));

    connect(ui->toTreeWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &XrefsDialog::onToTreeWidgetItemSelectionChanged);
    connect(ui->fromTreeWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &XrefsDialog::onFromTreeWidgetItemSelectionChanged);

    // Don't create recursive xref dialogs
    auto toContextMenu = ui->toTreeWidget->getItemContextMenu();
    connect(toContextMenu, &AddressableItemContextMenu::xrefsTriggered, this, &QWidget::close);
    auto fromContextMenu = ui->fromTreeWidget->getItemContextMenu();
    connect(fromContextMenu, &AddressableItemContextMenu::xrefsTriggered, this, &QWidget::close);

    connect(ui->toTreeWidget, &QAbstractItemView::doubleClicked, this, &QWidget::close);
    connect(ui->fromTreeWidget, &QAbstractItemView::doubleClicked, this, &QWidget::close);
}

XrefsDialog::~XrefsDialog() { }

QString XrefsDialog::normalizeAddr(const QString &addr) const
{
    QString ret = addr;
    if (addr.length() < 10) {
        ret = ret.mid(3).rightJustified(8, QLatin1Char('0'));
        ret.prepend(QLatin1Literal("0x"));
    }
    return ret;
}

void XrefsDialog::setupPreviewFont()
{
    ui->previewTextEdit->setFont(Config()->getBaseFont());
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

        selection.format.setBackground(ConfigColor("lineHighlight"));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->previewTextEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);

        ui->previewTextEdit->setExtraSelections(extraSelections);
    }
}

void XrefsDialog::onFromTreeWidgetItemSelectionChanged()
{
    auto index = ui->fromTreeWidget->currentIndex();
    if (!ui->fromTreeWidget->selectionModel()->hasSelection() || !index.isValid()) {
        return;
    }
    ui->toTreeWidget->clearSelection();
    updatePreview(fromModel.address(index));
}

void XrefsDialog::onToTreeWidgetItemSelectionChanged()
{
    auto index = ui->toTreeWidget->currentIndex();
    if (!ui->toTreeWidget->selectionModel()->hasSelection() || !index.isValid()) {
        return;
    }
    ui->fromTreeWidget->clearSelection();
    updatePreview(toModel.address(index));
}

void XrefsDialog::updatePreview(RVA addr)
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", true);
    tempConfig.set("scr.color", COLOR_MODE_16M);
    tempConfig.set("asm.lines", false);
    tempConfig.set("asm.bytes", false);

    // Use cmd because cmRaw cannot handle the output properly. Why?
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

void XrefsDialog::updateLabelsForVariables(QString name)
{
    ui->label_xTo->setText(tr("Writes to %1").arg(name));
    ui->label_xFrom->setText(tr("Reads from %1").arg(name));
}

void XrefsDialog::fillRefsForAddress(RVA addr, QString name, bool whole_function)
{
    setWindowTitle(tr("X-Refs for %1").arg(name));
    updateLabels(name);

    toModel.readForOffset(addr, true, whole_function);
    fromModel.readForOffset(addr, false, whole_function);

    // Adjust columns to content
    qhelpers::adjustColumns(ui->fromTreeWidget, fromModel.columnCount(), 0);
    qhelpers::adjustColumns(ui->toTreeWidget, toModel.columnCount(), 0);

    // Automatically select the first line
    if (!qhelpers::selectFirstItem(ui->toTreeWidget)) {
        qhelpers::selectFirstItem(ui->fromTreeWidget);
    }
}

void XrefsDialog::fillRefsForVariable(QString nameOfVariable, RVA offset)
{
    setWindowTitle(tr("X-Refs for %1").arg(nameOfVariable));
    updateLabelsForVariables(nameOfVariable);

    // Initialize Model
    toModel.readForVariable(nameOfVariable, true, offset);
    fromModel.readForVariable(nameOfVariable, false, offset);
    // Hide irrelevant column 1: which shows type
    ui->fromTreeWidget->hideColumn(XrefModel::Columns::TYPE);
    ui->toTreeWidget->hideColumn(XrefModel::Columns::TYPE);
    // Adjust columns to content
    qhelpers::adjustColumns(ui->fromTreeWidget, fromModel.columnCount(), 0);
    qhelpers::adjustColumns(ui->toTreeWidget, toModel.columnCount(), 0);

    // Automatically select the first line
    if (!qhelpers::selectFirstItem(ui->toTreeWidget)) {
        qhelpers::selectFirstItem(ui->fromTreeWidget);
    }
}

QString XrefModel::xrefTypeString(const QString &type)
{
    if (type == "CODE") {
        return QStringLiteral("Code");
    } else if (type == "CALL") {
        return QStringLiteral("Call");
    } else if (type == "DATA") {
        return QStringLiteral("Data");
    } else if (type == "STRING") {
        return QStringLiteral("String");
    }
    return type;
}


XrefModel::XrefModel(QObject *parent)
    : AddressableItemModel(parent)
{
}

void XrefModel::readForOffset(RVA offset, bool to, bool whole_function)
{
    beginResetModel();
    this->to = to;
    xrefs = Core()->getXRefs(offset, to, whole_function);
    endResetModel();
}

void XrefModel::readForVariable(QString nameOfVariable, bool write, RVA offset)
{
    beginResetModel();
    this->to = write;
    xrefs = Core()->getXRefsForVariable(nameOfVariable, write, offset);
    endResetModel();
}

int XrefModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return xrefs.size();
}

int XrefModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return Columns::COUNT;
}

QVariant XrefModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= xrefs.count()) {
        return QVariant();
    }

    const XrefDescription &xref = xrefs.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OFFSET:
            return to ? xref.from_str : xref.to_str;
        case TYPE:
            return xrefTypeString(xref.type);
        case CODE:
            if (to || xref.type != "DATA") {
                return Core()->disassembleSingleInstruction(xref.from);
            } else {
                return QString();
            }
        }
        return QVariant();
    case FlagDescriptionRole:
        return QVariant::fromValue(xref);
    default:
        break;
    }
    return QVariant();
}

QVariant XrefModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)

    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OFFSET:
            return tr("Address");
        case TYPE:
            return tr("Type");
        case CODE:
            return tr("Code");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA XrefModel::address(const QModelIndex &index) const
{
    const auto &xref = xrefs.at(index.row());
    return to ? xref.from : xref.to;
}
