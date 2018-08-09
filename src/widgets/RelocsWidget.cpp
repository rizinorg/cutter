#include <QTreeWidget>
#include "RelocsWidget.h"
#include "ui_RelocsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

RelocsModel::RelocsModel(QList<RelocDescription> *relocs, QObject *parent) :
    QAbstractTableModel(parent),
    relocs(relocs)
{}

int RelocsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : relocs->count();
}

int RelocsModel::columnCount(const QModelIndex&) const
{
    return RelocsModel::ColumnCount;
}

QVariant RelocsModel::data(const QModelIndex &index, int role) const
{
    const RelocDescription &reloc = relocs->at(index.row());
    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case RelocsModel::VAddrColumn:
            return RAddressString(reloc.vaddr);
        case RelocsModel::TypeColumn:
            return reloc.type;
        case RelocsModel::NameColumn:
            return reloc.name;
        default:
            break;
        }
        break;
    case RelocsModel::RelocDescriptionRole:
        return QVariant::fromValue(reloc);
    case RelocsModel::AddressRole:
        return reloc.vaddr;
    default:
        break;
    }
    return QVariant();
}

QVariant RelocsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole)
        switch (section)
        {
        case RelocsModel::VAddrColumn:
            return tr("Address");
        case RelocsModel::TypeColumn:
            return tr("Type");
        case RelocsModel::NameColumn:
            return tr("Name");
        }
    return QVariant();
}

void RelocsModel::beginReload()
{
    beginResetModel();
}

void RelocsModel::endReload()
{
    endResetModel();
}

RelocsProxyModel::RelocsProxyModel(RelocsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool RelocsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto reloc = index.data(RelocsModel::RelocDescriptionRole).value<RelocDescription>();

    return reloc.name.contains(filterRegExp());
}

bool RelocsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

    auto leftReloc = left.data(RelocsModel::RelocDescriptionRole).value<RelocDescription>();
    auto rightReloc = right.data(RelocsModel::RelocDescriptionRole).value<RelocDescription>();

    switch (left.column()) {
    case RelocsModel::VAddrColumn:
        return leftReloc.vaddr < rightReloc.vaddr;
    case RelocsModel::TypeColumn:
        return leftReloc.type < rightReloc.type;
    case RelocsModel::NameColumn:
        return leftReloc.name < rightReloc.name;
    default:
        break;
    }

    return false;
}

RelocsWidget::RelocsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::RelocsWidget),
    relocsModel(new RelocsModel(&relocs, this)),
    relocsProxyModel(new RelocsProxyModel(relocsModel, this))
{
    ui->setupUi(this);

    ui->relocsTreeView->setModel(relocsProxyModel);
    ui->relocsTreeView->sortByColumn(RelocsModel::NameColumn, Qt::AscendingOrder);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)),
            relocsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->relocsTreeView, SLOT(setFocus()));

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshRelocs()));
}

RelocsWidget::~RelocsWidget() {}

void RelocsWidget::on_relocsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    Core()->seek(index.data(RelocsModel::AddressRole).toLongLong());
}

void RelocsWidget::refreshRelocs()
{
    relocsModel->beginReload();
    relocs = Core()->getAllRelocs();
    relocsModel->endReload();
    qhelpers::adjustColumns(ui->relocsTreeView, 3, 0);
}

void RelocsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->relocsTreeView);
}
