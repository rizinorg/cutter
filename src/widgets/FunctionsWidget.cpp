#include "FunctionsWidget.h"
#include "ui_FunctionsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/XrefsDialog.h"

#include <algorithm>
#include <QMenu>
#include <QDebug>
#include <QString>
#include <QResource>
#include <QShortcut>

FunctionModel::FunctionModel(QList<FunctionDescription> *functions, QSet<RVA> *import_addresses, bool nested, QFont default_font, QFont highlight_font, QObject *parent)
    : QAbstractItemModel(parent),
      functions(functions),
      import_addresses(import_addresses),
      highlight_font(highlight_font),
      default_font(default_font),
      nested(nested),
      current_index(-1)

{
    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(seekChanged(RVA)));
    connect(Core(), SIGNAL(functionRenamed(const QString &, const QString &)), this, SLOT(functionRenamed(QString, QString)));
}

QModelIndex FunctionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    return createIndex(row, column, (quintptr)(parent.row() + 1)); // sub-nodes have id = function index + 1
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

    if (nested)
    {
        if (parent.internalId() == 0)
            return 3; // sub-nodes for nested functions
        return 0;
    }
    else
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
    return import_addresses->contains(addr);
}


QVariant FunctionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int function_index;
    bool subnode;
    if (index.internalId() != 0) // sub-node
    {
        function_index = index.parent().row();
        subnode = true;
    }
    else // root function node
    {
        function_index = index.row();
        subnode = false;
    }

    const FunctionDescription &function = functions->at(function_index);

    if (function_index >= functions->count())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        if (nested)
        {
            if (subnode)
            {
                switch (index.row())
                {
                case 0:
                    return tr("Offset: %1").arg(RAddressString(function.offset));
                case 1:
                    return tr("Size: %1").arg(RSizeString(function.size));
                case 2:
                    return tr("Import: %1").arg(functionIsImport(function.offset) ? tr("true") : tr("false"));
                default:
                    return QVariant();
                }
            }
            else
                return function.name;
        }
        else
        {
            switch (index.column())
            {
            case NameColumn:
                return function.name;
            case SizeColumn:
                return RSizeString(function.size);
            case OffsetColumn:
                return RAddressString(function.offset);
            default:
                return QVariant();
            }
        }

    case Qt::DecorationRole:
        if (import_addresses->contains(function.offset) &&
                (nested ? false : index.column() == ImportColumn))
            return QIcon(":/img/icons/import_light.svg");
        return QVariant();

    case Qt::FontRole:
        if (current_index == function_index)
            return highlight_font;
        return default_font;

    case Qt::ToolTipRole:
    {
        QList<QString> info = CutterCore::getInstance()->cmd("afi @ " + function.name).split("\n");
        if (info.length() > 2)
        {
            QString size = info[4].split(" ")[1];
            QString complex = info[8].split(" ")[1];
            QString bb = info[11].split(" ")[1];
            return QString("Summary:\n\n    Size: " + size +
                           "\n    Cyclomatic complexity: " + complex +
                           "\n    Basic blocks: " + bb +
                           "\n\nDisasm preview:\n\n" + CutterCore::getInstance()->cmd("pdi 10 @ " + function.name) +
                           "\nStrings:\n\n" + CutterCore::getInstance()->cmd("pdsf @ " + function.name));
        }
        return QVariant();
    }

    case Qt::ForegroundRole:
        if (functionIsImport(function.offset))
            return QVariant(ConfigColor("gui.imports"));
        return QVariant(QColor(Qt::black));

    case FunctionDescriptionRole:
        return QVariant::fromValue(function);

    case IsImportRole:
        return import_addresses->contains(function.offset);

    default:
        return QVariant();
    }
}

QVariant FunctionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (nested)
        {
            return tr("Name");
        }
        else
        {
            switch (section)
            {
            case NameColumn:
                return tr("Name");
            case SizeColumn:
                return tr("Size");
            case ImportColumn:
                return tr("Imp.");
            case OffsetColumn:
                return tr("Offset");
            default:
                return QVariant();
            }
        }
    }

    return QVariant();
}

void FunctionModel::beginReloadFunctions()
{
    beginResetModel();
}

void FunctionModel::endReloadFunctions()
{
    updateCurrentIndex();
    endResetModel();
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
    if (updateCurrentIndex())
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
}

bool FunctionModel::updateCurrentIndex()
{
    int index = -1;
    RVA offset = 0;

    RVA seek = Core()->getOffset();

    for (int i = 0; i < functions->count(); i++)
    {
        const FunctionDescription &function = functions->at(i);

        if (function.contains(seek)
                && function.offset >= offset)
        {
            offset = function.offset;
            index = i;
        }
    }

    bool changed = current_index != index;

    current_index = index;

    return changed;
}

void FunctionModel::functionRenamed(const QString &prev_name, const QString &new_name)
{
    for (int i = 0; i < functions->count(); i++)
    {
        FunctionDescription &function = (*functions)[i];
        if (function.name == prev_name)
        {
            function.name = new_name;
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
        }
    }
}




FunctionSortFilterProxyModel::FunctionSortFilterProxyModel(FunctionModel *source_model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool FunctionSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    FunctionDescription function = index.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    return function.name.contains(filterRegExp());
}

bool FunctionSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

    FunctionDescription left_function = left.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    FunctionDescription right_function = right.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();


    if (static_cast<FunctionModel *>(sourceModel())->isNested())
    {
        return left_function.name < right_function.name;
    }
    else
    {
        switch (left.column())
        {
        case FunctionModel::OffsetColumn:
            return left_function.offset < right_function.offset;
        case FunctionModel::SizeColumn:
            if (left_function.size != right_function.size)
                return left_function.size < right_function.size;
            break;
        case FunctionModel::ImportColumn:
        {
            bool left_is_import = left.data(FunctionModel::IsImportRole).toBool();
            bool right_is_import = right.data(FunctionModel::IsImportRole).toBool();
            if (!left_is_import && right_is_import)
                return true;
            break;
        }
        case FunctionModel::NameColumn:
            return left_function.name < right_function.name;
        default:
            return false;
        }

        return left_function.offset < right_function.offset;
    }
}

FunctionsWidget::FunctionsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FunctionsWidget),
    main(main)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

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

    function_model = new FunctionModel(&functions, &import_addresses, false, default_font, highlight_font, this);
    function_proxy_model = new FunctionSortFilterProxyModel(function_model, this);
    ui->functionsTreeView->setModel(function_proxy_model);
    ui->functionsTreeView->sortByColumn(FunctionModel::NameColumn, Qt::AscendingOrder);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)), function_proxy_model, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->functionsTreeView, SLOT(setFocus()));

    setScrollMode();

    // Set Functions context menu
    connect(ui->functionsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));

    connect(ui->functionsTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(functionsTreeView_doubleClicked(const QModelIndex &)));

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
    function_model->beginReloadFunctions();

    functions = CutterCore::getInstance()->getAllFunctions();

    import_addresses.clear();
    foreach (ImportDescription import, CutterCore::getInstance()->getAllImports())
        import_addresses.insert(import.plt);

    function_model->endReloadFunctions();

    // resize offset and size columns
    ui->functionsTreeView->resizeColumnToContents(0);
    ui->functionsTreeView->resizeColumnToContents(1);
    ui->functionsTreeView->resizeColumnToContents(2);
}

void FunctionsWidget::functionsTreeView_doubleClicked(const QModelIndex &index)
{
    FunctionDescription function = index.data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    CutterCore::getInstance()->seek(function.offset);
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
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

    // Create dialog
    CommentsDialog *c = new CommentsDialog(this);

    if (c->exec())
    {
        // Get new function name
        QString comment = c->getComment();
        // Rename function in r2 core
        CutterCore::getInstance()->setComment(function.offset, comment);
        // Seek to new renamed function
        CutterCore::getInstance()->seek(function.offset);
        // TODO: Refresh functions tree widget
    }
}

void FunctionsWidget::on_actionFunctionsRename_triggered()
{
    // Get selected item in functions tree view
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();

    // Create dialog
    RenameDialog *r = new RenameDialog(this);

    // Set function name in dialog
    r->setName(function.name);
    // If user accepted
    if (r->exec())
    {
        // Get new function name
        QString new_name = r->getName();
        // Rename function in r2 core
        CutterCore::getInstance()->renameFunction(function.name, new_name);

        // Scroll to show the new name in functions tree widget
        //
        // QAbstractItemView::EnsureVisible
        // QAbstractItemView::PositionAtTop
        // QAbstractItemView::PositionAtBottom
        // QAbstractItemView::PositionAtCenter
        //
        //ui->functionsTreeWidget->scrollToItem(selected_rows.first(), QAbstractItemView::PositionAtTop);
        // Seek to new renamed function
        CutterCore::getInstance()->seek(function.offset);
    }
}

void FunctionsWidget::on_actionFunctionsUndefine_triggered()
{
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    Core()->delFunction(function.offset);
}

void FunctionsWidget::on_action_References_triggered()
{
    // Get selected item in functions tree view
    FunctionDescription function = ui->functionsTreeView->selectionModel()->currentIndex().data(FunctionModel::FunctionDescriptionRole).value<FunctionDescription>();
    XrefsDialog *x = new XrefsDialog(this);
    x->fillRefsForAddress(function.offset, function.name, true);
    x->exec();
}

void FunctionsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (!function_model->isNested())
    {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    }
    else
    {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void FunctionsWidget::on_actionHorizontal_triggered()
{
    function_model->setNested(false);
    ui->functionsTreeView->setIndentation(8);
}

void FunctionsWidget::on_actionVertical_triggered()
{
    function_model->setNested(true);
    ui->functionsTreeView->setIndentation(20);
}

void FunctionsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible())
    {
        if (event->size().width() >= event->size().height())
        {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        }
        else
        {
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
