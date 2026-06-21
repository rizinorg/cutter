#include "FunctionsWidget.h"

#include "common/DisassemblyPreview.h"
#include "common/FunctionsTask.h"
#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "menus/AddressableItemContextMenu.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_ListDockWidget.h"

#include <QActionGroup>
#include <QBitmap>
#include <QDebug>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QPainter>
#include <QResource>
#include <QShortcut>
#include <QString>

namespace {

static const int kMaxTooltipWidth = 400;
static const int kMaxTooltipDisasmPreviewLines = 10;
static const int kMaxTooltipHighlightsLines = 5;

}

FunctionModel::FunctionModel(bool nested, const QFont &default_font, const QFont &highlight_font,
                             QObject *parent)
    : AddressableItemModel<>(parent),
      highlightFont(highlight_font),
      defaultFont(default_font),
      nested(nested),
      currentIndex(-1),
      iconFuncImpDark(":/img/icons/function_import_dark.svg"),
      iconFuncImpLight(":/img/icons/function_import_light.svg"),
      iconFuncMainDark(":/img/icons/function_main_dark.svg"),
      iconFuncMainLight(":/img/icons/function_main_light.svg"),
      iconFuncDark(":/img/icons/function_dark.svg"),
      iconFuncLight(":/img/icons/function_light.svg")

{
    connect(Core(), &CutterCore::seekChanged, this, &FunctionModel::seekChanged);
    connect(Core(), &CutterCore::functionRenamed, this, &FunctionModel::functionRenamed);
}

QModelIndex FunctionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0
    }

    return createIndex(row, column,
                       (quintptr)(parent.row() + 1)); // sub-nodes have id = function index + 1
}

QModelIndex FunctionModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0) {
        return QModelIndex();
    }

    if (index.internalId() == 0) { // root function node
        return QModelIndex();
    } else { // sub-node
        return this->index((int)(index.internalId() - 1), 0);
    }
}

int FunctionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return functions.count();
    }

    if (nested) {
        if (parent.internalId() == 0) {
            return ColumnCount - 1; // sub-nodes for nested functions
        }
        return 0;
    } else {
        return 0;
    }
}

int FunctionModel::columnCount(const QModelIndex & /*parent*/) const
{
    if (nested) {
        return 1;
    } else {
        return ColumnCount;
    }
}

bool FunctionModel::functionIsImport(ut64 addr) const
{
    return importAddresses.contains(addr);
}

bool FunctionModel::functionIsMain(ut64 addr) const
{
    return mainAdress == addr;
}

QVariant FunctionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int functionIndex;
    bool subnode;
    bool isDark;

    if (index.internalId() != 0) { // sub-node
        functionIndex = index.parent().row();
        subnode = true;
    } else { // root function node
        functionIndex = index.row();
        subnode = false;
    }

    const FunctionDescription &function = functions.at(functionIndex);

    if (functionIndex >= functions.count()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        if (nested) {
            if (subnode) {
                switch (index.row()) {
                case 0:
                    return tr("Offset: %1").arg(rzAddressString(function.offset));
                case 1:
                    return tr("Size: %1").arg(rzSizeString(function.linearSize));
                case 2:
                    return tr("Import: %1")
                            .arg(functionIsImport(function.offset) ? tr("true") : tr("false"));
                case 3:
                    return tr("Nargs: %1").arg(rzSizeString(function.nargs));
                case 4:
                    return tr("Nbbs: %1").arg(rzSizeString(function.nbbs));
                case 5:
                    return tr("Nlocals: %1").arg(rzSizeString(function.nlocals));
                case 6:
                    return tr("Call type: %1").arg(function.calltype);
                case 7:
                    return tr("Edges: %1").arg(function.edges);
                case 8:
                    return tr("StackFrame: %1").arg(function.stackframe);
                case 9:
                    return tr("Comment: %1").arg(Core()->getCommentAt(function.offset));
                default:
                    return QVariant();
                }
            } else {
                return function.name;
            }
        } else {
            switch (index.column()) {
            case NameColumn:
                return function.name;
            case SizeColumn:
                return QString::number(function.linearSize);
            case ImportColumn:
                return functionIsImport(function.offset) ? tr("true") : tr("false");
            case OffsetColumn:
                return rzAddressString(function.offset);
            case NargsColumn:
                return QString::number(function.nargs);
            case NlocalsColumn:
                return QString::number(function.nlocals);
            case NbbsColumn:
                return QString::number(function.nbbs);
            case CalltypeColumn:
                return function.calltype;
            case EdgesColumn:
                return QString::number(function.edges);
            case FrameColumn:
                return QString::number(function.stackframe);
            case CommentColumn:
                return Core()->getCommentAt(function.offset);
            default:
                return QVariant();
            }
        }

    case Qt::DecorationRole: {

        // Check if we aren't inside a tree view
        if (nested && subnode) {
            return QVariant();
        }

        if (index.column() == NameColumn) {
            isDark = Config()->windowColorIsDark();

            if (functionIsImport(function.offset)) {
                if (isDark) {
                    return iconFuncImpDark;
                }
                return iconFuncImpLight;

            } else if (functionIsMain(function.offset)) {
                if (isDark) {
                    return iconFuncMainDark;
                }
                return iconFuncMainLight;
            }

            if (isDark) {
                return iconFuncDark;
            }
            return iconFuncLight;
        }

        return QVariant();
    }

    case Qt::FontRole:
        if (currentIndex == functionIndex) {
            return highlightFont;
        }
        return defaultFont;

    case Qt::TextAlignmentRole:
        if (index.column() == 1) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::ToolTipRole: {

        const QStringList disasmPreview =
                Core()->getDisassemblyPreview(function.offset, kMaxTooltipDisasmPreviewLines);
        QStringList summary {};
        {
            auto core = Core()->lock();
            auto seeker = Core()->seekTemp(function.offset);
            auto strings = fromOwnedCharPtr(rz_core_print_disasm_strings(
                    core, RZ_CORE_DISASM_STRINGS_MODE_FUNCTION, 0, nullptr));
            summary = strings.split('\n', CUTTER_QT_SKIP_EMPTY_PARTS);
        }

        const QFont &fnt = Config()->getFont();
        const QFontMetrics fm { fnt };

        // elide long strings using current disasm font metrics
        QStringList highlights;
        for (const QString &s : std::as_const(summary)) {
            highlights << fm.elidedText(s, Qt::ElideRight, kMaxTooltipWidth);
            if (highlights.length() > kMaxTooltipHighlightsLines) {
                highlights << "...";
                break;
            }
        }
        if (disasmPreview.isEmpty() && highlights.isEmpty()) {
            return {};
        }

        QString toolTipContent =
                QString("<html><div style=\"font-family: '%1'; font-size: %2pt; white-space: "
                        "nowrap;\">")
                        .arg(fnt.family().toHtmlEscaped())
                        .arg(qMax(6, fnt.pointSize() - 1)); // slightly decrease font size, to
                                                            // keep more text in the same box

        if (!disasmPreview.isEmpty()) {
            toolTipContent += tr("<div style=\"margin-bottom: 10px;\"><strong>Disassembly "
                                 "preview</strong>:<br>%1</div>")
                                      .arg(disasmPreview.join("<br>"));
        }

        if (!highlights.isEmpty()) {
            toolTipContent += tr("<div><strong>Highlights</strong>:<br>%1</div>")
                                      .arg(highlights.join(QLatin1Char('\n'))
                                                   .toHtmlEscaped()
                                                   .replace(QLatin1Char('\n'), "<br>"));
        }
        toolTipContent += "</div></html>";
        return toolTipContent;
    }

    case Qt::ForegroundRole:
        if (functionIsImport(function.offset)) {
            return QVariant(ConfigColor("gui.imports"));
        } else if (functionIsMain(function.offset)) {
            return QVariant(ConfigColor("gui.main"));
        } else if (function.name.startsWith("flirt.")) {
            return QVariant(ConfigColor("gui.flirt"));
        }

        return QVariant(this->property("color"));

    case functionDescriptionRole:
        return QVariant::fromValue(function);

    case isImportRole:
        return importAddresses.contains(function.offset);

    default:
        return {};
    }
}

QVariant FunctionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (nested) {
            return tr("Name");
        } else {
            switch (section) {
            case NameColumn:
                return tr("Name");
            case SizeColumn:
                return tr("Size");
            case ImportColumn:
                return tr("Import");
            case OffsetColumn:
                return tr("Offset");
            case NargsColumn:
                return tr("Nargs");
            case NlocalsColumn:
                return tr("Nlocals");
            case NbbsColumn:
                return tr("Nbbs");
            case CalltypeColumn:
                return tr("Call type");
            case EdgesColumn:
                return tr("Edges");
            case FrameColumn:
                return tr("StackFrame");
            case CommentColumn:
                return tr("Comment");
            default:
                return QVariant();
            }
        }
    }

    return QVariant();
}

void FunctionModel::setNested(bool nested)
{
    beginResetModel();
    this->nested = nested;
    updateCurrentIndex();
    endResetModel();
}

RVA FunctionModel::address(const QModelIndex &index) const
{
    auto function = data(index, functionDescriptionRole).value<FunctionDescription>();
    return function.offset;
}

QString FunctionModel::name(const QModelIndex &index) const
{
    auto function = data(index, functionDescriptionRole).value<FunctionDescription>();
    return function.name;
}

void FunctionModel::seekChanged(RVA)
{
    const int previousIndex = currentIndex;
    if (updateCurrentIndex()) {
        if (previousIndex >= 0) {
            emit dataChanged(index(previousIndex, 0), index(previousIndex, columnCount() - 1));
        }
        if (currentIndex >= 0) {
            emit dataChanged(index(currentIndex, 0), index(currentIndex, columnCount() - 1));
        }
    }
}

bool FunctionModel::updateCurrentIndex()
{
    int index = -1;
    RVA offset = 0;

    const RVA seek = Core()->getOffset();

    for (int i = 0; i < functions.count(); i++) {
        const FunctionDescription &function = functions.at(i);

        if (function.contains(seek) && function.offset >= offset) {
            offset = function.offset;
            index = i;
        }
    }

    const bool changed = currentIndex != index;

    currentIndex = index;

    return changed;
}

void FunctionModel::functionRenamed(const RVA offset, const QString &new_name)
{
    for (int i = 0; i < functions.count(); i++) {
        FunctionDescription &function = functions[i];
        if (function.offset == offset) {
            function.name = new_name;
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}

FunctionSortFilterProxyModel::FunctionSortFilterProxyModel(FunctionModel *source_model,
                                                           QObject *parent)
    : AddressableFilterProxyModel(source_model, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool FunctionSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto function =
            index.data(FunctionModel::functionDescriptionRole).value<FunctionDescription>();

    return qhelpers::filterStringContains(function.name, this);
}

bool FunctionSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid()) {
        return false;
    }

    if (left.parent().isValid() || right.parent().isValid()) {
        return false;
    }

    const auto leftFunction =
            left.data(FunctionModel::functionDescriptionRole).value<FunctionDescription>();
    const auto rightFunction =
            right.data(FunctionModel::functionDescriptionRole).value<FunctionDescription>();

    if (static_cast<FunctionModel *>(sourceModel())->isNested()) {
        return leftFunction.name < rightFunction.name;
    } else {
        switch (left.column()) {
        case FunctionModel::OffsetColumn:
            return leftFunction.offset < rightFunction.offset;
        case FunctionModel::SizeColumn:
            if (leftFunction.linearSize != rightFunction.linearSize) {
                return leftFunction.linearSize < rightFunction.linearSize;
            }
            break;
        case FunctionModel::ImportColumn: {
            const bool leftIsImport = left.data(FunctionModel::isImportRole).toBool();
            const bool rightIsImport = right.data(FunctionModel::isImportRole).toBool();
            if (!leftIsImport && rightIsImport) {
                return true;
            }
            break;
        }
        case FunctionModel::NameColumn:
            return leftFunction.name < rightFunction.name;
        case FunctionModel::NargsColumn:
            if (leftFunction.nargs != rightFunction.nargs) {
                return leftFunction.nargs < rightFunction.nargs;
            }
            break;
        case FunctionModel::NlocalsColumn:
            if (leftFunction.nlocals != rightFunction.nlocals) {
                return leftFunction.nlocals < rightFunction.nlocals;
            }
            break;
        case FunctionModel::NbbsColumn:
            if (leftFunction.nbbs != rightFunction.nbbs) {
                return leftFunction.nbbs < rightFunction.nbbs;
            }
            break;
        case FunctionModel::CalltypeColumn:
            return leftFunction.calltype < rightFunction.calltype;
        case FunctionModel::EdgesColumn:
            if (leftFunction.edges != rightFunction.edges) {
                return leftFunction.edges < rightFunction.edges;
            }
            break;
        case FunctionModel::FrameColumn:
            if (leftFunction.stackframe != rightFunction.stackframe) {
                return leftFunction.stackframe < rightFunction.stackframe;
            }
            break;
        case FunctionModel::CommentColumn:
            return Core()->getCommentAt(leftFunction.offset)
                    < Core()->getCommentAt(rightFunction.offset);
        default:
            return false;
        }

        return leftFunction.offset < rightFunction.offset;
    }
}

FunctionsWidget::FunctionsWidget(MainWindow *main)
    : ListDockWidget(main),
      titleContextMenu(new QMenu(this)),
      actionRename(tr("Rename"), this),
      actionUndefine(tr("Undefine"), this),
      actionHorizontal(tr("Horizontal"), this),
      actionVertical(tr("Vertical"), this)
{
    setWindowTitle(tr("Functions"));
    setObjectName("FunctionsWidget");

    setTooltipStylesheet();
    connect(Config(), &Configuration::colorsUpdated, this, &FunctionsWidget::setTooltipStylesheet);

    const QFontInfo fontInfo = ui->treeView->fontInfo();
    const QFont defaultFont = QFont(fontInfo.family(), fontInfo.pointSize());
    const QFont highlightFont = QFont(fontInfo.family(), fontInfo.pointSize(), QFont::Bold);

    functionModel = new FunctionModel(false, defaultFont, highlightFont, this);
    functionProxyModel = new FunctionSortFilterProxyModel(functionModel, this);

    setModels(functionProxyModel);
    ui->treeView->sortByColumn(FunctionModel::NameColumn, Qt::AscendingOrder);
    ui->treeView->setExpandsOnDoubleClick(false);

    auto viewTypeGroup = new QActionGroup(titleContextMenu);
    actionHorizontal.setCheckable(true);
    actionHorizontal.setActionGroup(viewTypeGroup);
    connect(&actionHorizontal, &QAction::toggled, this,
            &FunctionsWidget::onActionHorizontalToggled);
    actionVertical.setCheckable(true);
    actionVertical.setActionGroup(viewTypeGroup);
    connect(&actionVertical, &QAction::toggled, this, &FunctionsWidget::onActionVerticalToggled);
    titleContextMenu->addActions(viewTypeGroup->actions());

    Shortcuts()->setupAction(actionRename, "Functions.rename");
    actionRename.setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(&actionRename, &QAction::triggered, this,
            &FunctionsWidget::onActionFunctionsRenameTriggered);
    connect(&actionUndefine, &QAction::triggered, this,
            &FunctionsWidget::onActionFunctionsUndefineTriggered);

    auto itemContextMenu = ui->treeView->getItemContextMenu();
    itemContextMenu->toggleBreakpointAction(true);
    itemContextMenu->addSeparator();
    itemContextMenu->addAction(&actionRename);
    itemContextMenu->addAction(&actionUndefine);
    itemContextMenu->setWholeFunction(true);

    ui->treeView->addActions(itemContextMenu->actions());

    // Use a custom context menu on the dock title bar
    if (!functionModel->isNested()) {
        actionHorizontal.setChecked(true);
    } else {
        actionVertical.setChecked(true);
    }
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this,
            &FunctionsWidget::showTitleContextMenu);

    connect(Core(), &CutterCore::functionsChanged, this, &FunctionsWidget::refreshTree);
    connect(Core(), &CutterCore::codeRebased, this, &FunctionsWidget::refreshTree);
    connect(Core(), &CutterCore::refreshAll, this, &FunctionsWidget::refreshTree);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(functionModel, FunctionModel::CommentColumn); });

    // Save the width of function name column so it's preserved when
    // switching from horizontal->vertical->horizontal layout
    connect(ui->treeView->header(), &QHeaderView::sectionResized, this,
            [this](int index, int, int newSize) {
                if (index == FunctionModel::NameColumn && !functionModel->isNested()) {
                    maxFunctionNameWidth = newSize;
                }
            });

    auto updateNameColumnWidth = [this] {
        const bool truncate = Config()->getTruncateFunctionNameCol();
        if (truncate) {
            maxFunctionNameWidth = Config()->getFunctionNameColWidth();
        }

        qhelpers::adjustColumn(ui->treeView, FunctionModel::NameColumn,
                               truncate ? maxFunctionNameWidth : -1);
    };
    connect(Config(), &Configuration::functionsOptionsChanged, this, updateNameColumnWidth);
    maxFunctionNameWidth = Config()->getFunctionNameColWidth();
}

FunctionsWidget::~FunctionsWidget() {}

void FunctionsWidget::refreshTree()
{
    if (task) {
        task->wait();
    }

    task = std::shared_ptr<FunctionsTask>(new FunctionsTask());
    connect(task.get(), &FunctionsTask::fetchFinished, this,
            [this](const QList<FunctionDescription> &functions) {
                functionModel->beginResetModel();

                functionModel->functions = functions;

                functionModel->importAddresses.clear();
                for (const ImportDescription &import : Core()->getAllImports()) {
                    functionModel->importAddresses.insert(import.plt);
                }

                functionModel->mainAdress = RVA_INVALID;
                RzCoreLocked core(Core());
                const RzBinFile *bf = rz_bin_cur(core->bin);
                if (bf) {
                    const RzBinAddr *binmain =
                            rz_bin_object_get_special_symbol(bf->o, RZ_BIN_SPECIAL_SYMBOL_MAIN);
                    if (binmain) {
                        const int va = core->io->va || core->bin->is_debugger;
                        functionModel->mainAdress = va
                                ? rz_bin_object_addr_with_base(bf->o, binmain->vaddr)
                                : binmain->paddr;
                    }
                }

                functionModel->updateCurrentIndex();
                functionModel->endResetModel();

                // set the initial item count
                ui->quickFilterView->setItemCount(functionProxyModel->rowCount());

                // resize offset and size columns
                qhelpers::adjustColumns(ui->treeView, 1, 3, 0);

                // resize name column
                qhelpers::adjustColumn(ui->treeView, FunctionModel::NameColumn,
                                       Config()->getTruncateFunctionNameCol() ? maxFunctionNameWidth
                                                                              : -1);
            });
    Core()->getAsyncTaskManager()->start(task);
}

void FunctionsWidget::changeSizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver)
{
    ui->dockWidgetContents->setSizePolicy(hor, ver);
}

void FunctionsWidget::onActionFunctionsRenameTriggered()
{
    // Get selected item in functions tree view
    const auto function = ui->treeView->selectionModel()
                                  ->currentIndex()
                                  .data(FunctionModel::functionDescriptionRole)
                                  .value<FunctionDescription>();

    bool ok;
    // Create dialog
    const QString newName =
            QInputDialog::getText(this, tr("Rename function %1").arg(function.name),
                                  tr("Function name:"), QLineEdit::Normal, function.name, &ok);
    // If user accepted
    if (ok && !newName.isEmpty()) {
        // Rename function in rizin core
        Core()->renameFunction(function.offset, newName);

        // Seek to new renamed function
        Core()->seekAndShow(function.offset);
    }
}

void FunctionsWidget::onActionFunctionsUndefineTriggered()
{
    const auto selection = ui->treeView->selectionModel()->selection().indexes();
    QSet<RVA> offsets;
    for (const auto &index : selection) {
        offsets.insert(functionProxyModel->address(index));
    }
    for (const RVA &offset : offsets) {
        Core()->delFunction(offset);
    }
}

void FunctionsWidget::showTitleContextMenu(const QPoint &pt)
{
    titleContextMenu->exec(this->mapToGlobal(pt));
}

void FunctionsWidget::onActionHorizontalToggled(bool enable)
{
    if (enable) {
        Config()->setFunctionsWidgetLayout("horizontal");
        functionModel->setNested(false);
        ui->treeView->setIndentation(8);

        qhelpers::adjustColumn(ui->treeView, FunctionModel::NameColumn,
                               Config()->getTruncateFunctionNameCol() ? maxFunctionNameWidth : -1);
    }
}

void FunctionsWidget::onActionVerticalToggled(bool enable)
{
    if (enable) {
        Config()->setFunctionsWidgetLayout("vertical");
        functionModel->setNested(true);
        ui->treeView->setIndentation(20);

        qhelpers::adjustColumn(ui->treeView, FunctionModel::NameColumn);
    }
}

void FunctionsWidget::setTooltipStylesheet()
{
    setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
}
