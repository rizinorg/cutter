#include "ExportsWidget.h"
#include "ui_ExportsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

ExportsModel::ExportsModel(QList<ExportDescription> *exports, QObject *parent)
    : QAbstractListModel(parent),
      exports(exports)
{
}

int ExportsModel::rowCount(const QModelIndex &) const
{
    return exports->count();
}

int ExportsModel::columnCount(const QModelIndex &) const
{
    return ExportsModel::ColumnCount;
}

QVariant ExportsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= exports->count())
        return QVariant();

    const ExportDescription &exp = exports->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ExportsModel::OffsetColumn:
            return RAddressString(exp.vaddr);
        case ExportsModel::SizeColumn:
            return RSizeString(exp.size);
        case ExportsModel::TypeColumn:
            return exp.type;
        case ExportsModel::NameColumn:
            return exp.name;
        default:
            return QVariant();
        }
    case ExportsModel::ExportDescriptionRole:
        return QVariant::fromValue(exp);
    default:
        return QVariant();
    }
}

QVariant ExportsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case ExportsModel::OffsetColumn:
            return tr("Address");
        case ExportsModel::SizeColumn:
            return tr("Size");
        case ExportsModel::TypeColumn:
            return tr("Type");
        case ExportsModel::NameColumn:
            return tr("Name");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

void ExportsModel::beginReloadExports()
{
    beginResetModel();
}

void ExportsModel::endReloadExports()
{
    endResetModel();
}

ExportsSortFilterProxyModel::ExportsSortFilterProxyModel(ExportsModel *source_model,
                                                         QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ExportsSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    ExportDescription exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    return exp.name.contains(filterRegExp());
}

bool ExportsSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ExportDescription left_exp = left.data(
                                     ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    ExportDescription right_exp = right.data(
                                      ExportsModel::ExportDescriptionRole).value<ExportDescription>();

    switch (left.column()) {
    case ExportsModel::SizeColumn:
        if (left_exp.size != right_exp.size)
            return left_exp.size < right_exp.size;
    // fallthrough
    case ExportsModel::OffsetColumn:
        if (left_exp.vaddr != right_exp.vaddr)
            return left_exp.vaddr < right_exp.vaddr;
    // fallthrough
    case ExportsModel::NameColumn:
        return left_exp.name < right_exp.name;
    case ExportsModel::TypeColumn:
        if (left_exp.type != right_exp.type)
            return left_exp.type < right_exp.type;
    default:
        break;
    }

    // fallback
    return left_exp.vaddr < right_exp.vaddr;
}



ExportsWidget::ExportsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ExportsWidget)
{
    ui->setupUi(this);

    exportsModel = new ExportsModel(&exports, this);
    exportsProxyModel = new ExportsSortFilterProxyModel(exportsModel, this);
    ui->exportsTreeView->setModel(exportsProxyModel);
    ui->exportsTreeView->sortByColumn(ExportsModel::OffsetColumn, Qt::AscendingOrder);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)),
            exportsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->exportsTreeView, SLOT(setFocus()));

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshExports()));
}

ExportsWidget::~ExportsWidget() {}

void ExportsWidget::refreshExports()
{
    exportsModel->beginReloadExports();
    exports = Core()->getAllExports();
    exportsModel->endReloadExports();

    qhelpers::adjustColumns(ui->exportsTreeView, 3, 0);
}


void ExportsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->exportsTreeView);
}

void ExportsWidget::on_exportsTreeView_doubleClicked(const QModelIndex &index)
{
    ExportDescription exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    Core()->seek(exp.vaddr);
}
