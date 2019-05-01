#include "FunctionsWidget.h"
#include "ui_FunctionsWidget.h"

#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/XrefsDialog.h"
#include "common/FunctionsTask.h"
#include "common/TempConfig.h"

#include <algorithm>
#include <QMenu>
#include <QDebug>
#include <QString>
#include <QResource>
#include <QShortcut>
#include <QJsonArray>
#include <QJsonObject>

namespace {

static const int kMaxTooltipWidth = 400;
static const int kMaxTooltipDisasmPreviewLines = 10;
static const int kMaxTooltipHighlightsLines = 5;

}

FunctionModel::FunctionModel(QList<FunctionDescription> *functions, QSet<RVA> *importAddresses,
                             ut64 *mainAdress, bool nested, QFont default_font, QFont highlight_font, QObject *parent)
    : QAbstractItemModel(parent),
      functions(functions),
      importAddresses(importAddresses),
      mainAdress(mainAdress),
      highlightFont(highlight_font),
      defaultFont(default_font),
      nested(nested),
      currentIndex(-1)

{
    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(seekChanged(RVA)));
    connect(Core(), SIGNAL(functionRenamed(const QString &, const QString &)), this,
            SLOT(functionRenamed(QString, QString)));
}

QModelIndex FunctionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column,
                       (quintptr)(parent.row() + 1)); // sub-nodes have id = function index + 1
}

QModelIndex FunctionModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return QModelIndex();

    if (index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId() - 1), 0);
}

int FunctionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return functions->count();

    if (nested) {
        if (parent.internalId() == 0)
            return ColumnCount - 1; // sub-nodes for nested functions
        return 0;
    } else
        return 0;
}

int FunctionModel::columnCount(const QModelIndex &/*parent*/) const
{
    if (nested)
        return 1;
    else
        return ColumnCount;
}

bool FunctionModel::functionIsImport(ut64 addr) const
{
    return importAddresses->contains(addr);
}

bool FunctionModel::functionIsMain(ut64 addr) const
{
    return *mainAdress == addr;
}

QVariant FunctionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int function_index;
    bool subnode;
    if (index.internalId() != 0) { // sub-node
        function_index = index.parent().row();
        subnode = true;
    } else { // root function node
        function_index = index.row();
        subnode = false;
    }

    const FunctionDescription &function = functions->at(function_index);

    if (function_index >= functions->count())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if (nested) {
            if (subnode) {
                switch (index.row()) {
                case 0:
                    return tr("Offset: %1").arg(RAddressString(function.offset));
                case 1:
                    return tr("Size: %1").arg(RSizeString(function.size));
                case 2:
                    return tr("Import: %1").arg(functionIsImport(function.offset) ? tr("true") : tr("false"));
                case 3:
                    return tr("Nargs: %1").arg(RSizeString(function.nargs));
                case 4:
                    return tr("Nbbs: %1").arg(RSizeString(function.nbbs));
                case 5:
                    return tr("Nlocals: %1").arg(RSizeString(function.nlocals));
                case 6:
                    return tr("Call type: %1").arg(function.calltype);
                case 7:
                    return tr("Edges: %1").arg(function.edges);
                case 8:
                    return tr("StackFrame: %1").arg(function.stackframe);
                default:
                    return QVariant();
                }
            } else
                return function.name;
        } else {
            switch (index.column()) {
            case NameColumn:
                return function.name;
            case SizeColumn:
                return function.size;
            case OffsetColumn:
                return RAddressString(function.offset);
            case NargsColumn:
                return function.nargs;
            case NlocalsColumn:
                return function.nlocals;
            case NbbsColumn:
                return function.nbbs;
            case CalltypeColumn:
                return function.calltype;
            case EdgesColumn:
                return function.edges;
            case FrameColumn:
                return function.stackframe;
            default:
                return QVariant();
            }
        }

    case Qt::DecorationRole:
        if (importAddresses->contains(function.offset) &&
                (nested ? false : index.column() == ImportColumn)) {
            const static QIcon importIcon(":/img/icons/import_light.svg");
            return importIcon;
        }
        return QVariant();

    case Qt::FontRole:
        if (currentIndex == function_index)
            return highlightFont;
        return defaultFont;

    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::ToolTipRole: {

        QStringList disasmPreview = Core()->getDisassemblyPreview(function.offset, kMaxTooltipDisasmPreviewLines);
        const QStringList &summary = Core()->cmdList(QString("pdsf @ %1").arg(function.offset));
        const QFont &fnt = Config()->getFont();
        QFontMetrics fm{ fnt };

        // elide long strings using current disasm font metrics
        QStringList highlights;
        for (const QString &s : summary) {
            highlights << fm.elidedText(s, Qt::ElideRight, kMaxTooltipWidth);
            if (highlights.length() > kMaxTooltipHighlightsLines) {
                highlights << "...";
                break;
            }
        }
        if (disasmPreview.isEmpty() && highlights.isEmpty())
            return QVariant();

        QString toolTipContent = QString("<html><div style=\"font-family: %1; font-size: %2pt; white-space: nowrap;\">")
                .arg(fnt.family())
                .arg(qMax(6, fnt.pointSize() - 1)); // slightly decrease font size, to keep more text in the same box

        if (!disasmPreview.isEmpty())
            toolTipContent += tr("<div style=\"margin-bottom: 10px;\"><strong>Disassembly preview</strong>:<br>%1</div>")
                .arg(disasmPreview.join("<br>"));

        if (!highlights.isEmpty()) {
            toolTipContent += tr("<div><strong>Highlights</strong>:<br>%1</div>")
                .arg(highlights.join(QLatin1Char('\n')).toHtmlEscaped().replace(QLatin1Char('\n'), "<br>"));
        }
        toolTipContent += "</div></html>";
        return toolTipContent;
    }

    case Qt::ForegroundRole:
        if (functionIsImport(function.offset))
            return QVariant(ConfigColor("gui.imports"));
        if (functionIsMain(function.offset))
            return QVariant(ConfigColor("gui.main"));
        return QVariant(this->property("color"));

    case FunctionDescriptionRole:
        return QVariant::fromValue(function);

    case IsImportRole:
        return importAddresses->contains(function.offset);

    default:
        return QVariant();
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
                return tr("Imp.");
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

void FunctionModel::seekChanged(RVA)
{
    int previousIndex = currentIndex;
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

    RVA seek = Core()->getOffset();

    for (int i = 0; i < functions->count(); i++) {
        const FunctionDescription &function = functions->at(i);

        if (function.contains(seek)
                && function.offset >= offset) {
            offset = function.offset;
            index = i;
        }
    }

    bool changed = currentIndex != index;

    currentIndex = index;

    return changed;
}

void FunctionModel::functionRenamed(const QString &prev_name, const QString &new_name)
{
    for (int i = 0; i < functions->count(); i++) {
        FunctionDescription &function = (*functions)[i];
        if (function.name == prev_name) {
            function.name = new_name;
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}

FunctionSortFilterProxyModel::FunctionSortFilterProxyModel(FunctionModel *source_model,
                                                           QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool FunctionSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    FunctionDescription function = index.data(
                                       FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    return function.name.contains(filterRegExp());
}

bool FunctionSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

    FunctionDescription left_function = left.data(
                                            FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    FunctionDescription right_function = right.data(
                                             FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();


    if (static_cast<FunctionModel *>(sourceModel())->isNested()) {
        return left_function.name < right_function.name;
    } else {
        switch (left.column()) {
        case FunctionModel::OffsetColumn:
            return left_function.offset < right_function.offset;
        case FunctionModel::SizeColumn:
            if (left_function.size != right_function.size)
                return left_function.size < right_function.size;
            break;
        case FunctionModel::ImportColumn: {
            bool left_is_import = left.data(FunctionModel::IsImportRole).toBool();
            bool right_is_import = right.data(FunctionModel::IsImportRole).toBool();
            if (!left_is_import && right_is_import)
                return true;
            break;
        }
        case FunctionModel::NameColumn:
            return left_function.name < right_function.name;
        case FunctionModel::NargsColumn:
            if (left_function.nargs != right_function.nargs)
                return left_function.nargs < right_function.nargs;
            break;
        case FunctionModel::NlocalsColumn:
            if (left_function.nlocals != right_function.nlocals)
                return left_function.nlocals < right_function.nlocals;
            break;
        case FunctionModel::NbbsColumn:
            if (left_function.nbbs != right_function.nbbs)
                return left_function.nbbs < right_function.nbbs;
            break;
        case FunctionModel::CalltypeColumn:
            return left_function.calltype < right_function.calltype;
        case FunctionModel::EdgesColumn:
            if (left_function.edges != right_function.edges)
                return left_function.edges < right_function.edges;
            break;
        case FunctionModel::FrameColumn:
            if (left_function.stackframe != right_function.stackframe)
                return left_function.stackframe < right_function.stackframe;
            break;
        default:
            return false;
        }

        return left_function.offset < right_function.offset;
    }
}

FunctionsWidget::FunctionsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::FunctionsWidget),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);

    // Add Status Bar footer 
    tree->addStatusBar(ui->verticalLayout);

    // Radare core found in:
    this->main = main;
    setTooltipStylesheet();
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(setTooltipStylesheet()));


    // leave the filter visible by default so users know it exists
    //ui->filterLineEdit->setVisible(false);

    // Ctrl-F to show/hide the filter entry
    QShortcut *search_shortcut = new QShortcut(QKeySequence::Find, this);
    connect(search_shortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    search_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clear_shortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QFontInfo font_info = ui->functionsTreeView->fontInfo();
    QFont default_font = QFont(font_info.family(), font_info.pointSize());
    QFont highlight_font = QFont(font_info.family(), font_info.pointSize(), QFont::Bold);

    functionModel = new FunctionModel(&functions, &importAddresses, &mainAdress, false, default_font,
                                      highlight_font, this);
    functionProxyModel = new FunctionSortFilterProxyModel(functionModel, this);
    ui->functionsTreeView->setModel(functionProxyModel);
    ui->functionsTreeView->sortByColumn(FunctionModel::NameColumn, Qt::AscendingOrder);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)), functionProxyModel,
            SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->functionsTreeView, SLOT(setFocus()));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(functionProxyModel->rowCount());
    });
    
    setScrollMode();

    // Set Functions context menu
    connect(ui->functionsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));

    connect(ui->functionsTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this,
            SLOT(onFunctionsDoubleClicked(const QModelIndex &)));

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(refreshTree()));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTree()));
}

FunctionsWidget::~FunctionsWidget() {}

void FunctionsWidget::refreshTree()
{
    if (task) {
        task->wait();
    }

    task = QSharedPointer<FunctionsTask>(new FunctionsTask());
    connect(task.data(), &FunctionsTask::fetchFinished,
    this, [this] (const QList<FunctionDescription> &functions) {
        functionModel->beginResetModel();

        this->functions = functions;

        importAddresses.clear();
        for (const ImportDescription &import : Core()->getAllImports()) {
            importAddresses.insert(import.plt);
        }

        mainAdress = (ut64)Core()->cmdj("iMj").object()["vaddr"].toInt();

        functionModel->updateCurrentIndex();
        functionModel->endResetModel();

        // resize offset and size columns
        qhelpers::adjustColumns(ui->functionsTreeView, 3, 0);

        tree->showItemsNumber(functionProxyModel->rowCount());
    });
    Core()->getAsyncTaskManager()->start(task);
}

void FunctionsWidget::changeSizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver)
{
    ui->dockWidgetContents->setSizePolicy(hor, ver);
}

void FunctionsWidget::onFunctionsDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    FunctionDescription function = index.data(
                                       FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    Core()->seek(function.offset);
}

void FunctionsWidget::showFunctionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(ui->functionsTreeView);
    menu->clear();
    menu->addAction(ui->actionDisasAdd_comment);
    menu->addAction(ui->actionFunctionsRename);
    menu->addAction(ui->actionFunctionsUndefine);
    menu->addSeparator();
    menu->addAction(ui->action_References);

    menu->exec(ui->functionsTreeView->mapToGlobal(pt));

    delete menu;
}

void FunctionsWidget::on_actionDisasAdd_comment_triggered()
{
    // Get selected item in functions tree view
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(
                                       FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

    // Create dialog
    CommentsDialog c(this);

    if (c.exec()) {
        // Get new function name
        QString comment = c.getComment();
        // Rename function in r2 core
        Core()->setComment(function.offset, comment);
        // Seek to new renamed function
        Core()->seek(function.offset);
        // TODO: Refresh functions tree widget
    }
}

void FunctionsWidget::on_actionFunctionsRename_triggered()
{
    // Get selected item in functions tree view
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(
                                       FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

    // Create dialog
    RenameDialog r(this);

    // Set function name in dialog
    r.setName(function.name);
    // If user accepted
    if (r.exec()) {
        // Get new function name
        QString new_name = r.getName();

        // Rename function in r2 core
        Core()->renameFunction(function.name, new_name);

        // Seek to new renamed function
        Core()->seek(function.offset);
    }
}

void FunctionsWidget::on_actionFunctionsUndefine_triggered()
{
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(
                                       FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    Core()->delFunction(function.offset);
}

void FunctionsWidget::on_action_References_triggered()
{
    // Get selected item in functions tree view
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(
                                       FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    XrefsDialog x(nullptr);
    x.fillRefsForAddress(function.offset, function.name, true);
    x.exec();
}

void FunctionsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (!functionModel->isNested()) {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    } else {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void FunctionsWidget::on_actionHorizontal_triggered()
{
    functionModel->setNested(false);
    ui->functionsTreeView->setIndentation(8);
}

void FunctionsWidget::on_actionVertical_triggered()
{
    functionModel->setNested(true);
    ui->functionsTreeView->setIndentation(20);
}

void FunctionsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible()) {
        if (event->size().width() >= event->size().height()) {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        } else {
            // Set vertical view (Tree)
            on_actionVertical_triggered();
        }
    }
    QDockWidget::resizeEvent(event);
}

void FunctionsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->functionsTreeView);
}

/**
 * @brief a SLOT to set the stylesheet for a tooltip
 */ 
void FunctionsWidget::setTooltipStylesheet()
{
    setStyleSheet(QString("QToolTip { border-width: 1px; max-width: %1px;" \
                          "opacity: 230; background-color: %2;" \
                          "color: %3; border-color: %3;}")
                  .arg(kMaxTooltipWidth)
                  .arg(Config()->getColor("gui.tooltip.background").name())
                  .arg(Config()->getColor("gui.tooltip.foreground").name()));
}
