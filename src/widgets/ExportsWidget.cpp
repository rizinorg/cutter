#include "ExportsWidget.h"
#include "ui_ListDockWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "WidgetShortcuts.h"

#include <QShortcut>

ExportsModel::ExportsModel(QList<ExportDescription> *exports, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent),
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

RVA ExportsModel::address(const QModelIndex &index) const
{
    const ExportDescription &exp = exports->at(index.row());
    return exp.vaddr;
}

QString ExportsModel::name(const QModelIndex &index) const
{
    const ExportDescription &exp = exports->at(index.row());
    return exp.name;
}

ExportsProxyModel::ExportsProxyModel(ExportsModel *source_model, QObject *parent)
    : AddressableFilterProxyModel(source_model, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ExportsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();

    return exp.name.contains(filterRegExp());
}

bool ExportsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftExp = left.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    auto rightExp = right.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();

    switch (left.column()) {
    case ExportsModel::SizeColumn:
        if (leftExp.size != rightExp.size)
            return leftExp.size < rightExp.size;
    // fallthrough
    case ExportsModel::OffsetColumn:
        if (leftExp.vaddr != rightExp.vaddr)
            return leftExp.vaddr < rightExp.vaddr;
    // fallthrough
    case ExportsModel::NameColumn:
        return leftExp.name < rightExp.name;
    case ExportsModel::TypeColumn:
        if (leftExp.type != rightExp.type)
            return leftExp.type < rightExp.type;
    default:
        break;
    }

    // fallback
    return leftExp.vaddr < rightExp.vaddr;
}

ExportsWidget::ExportsWidget(MainWindow *main, QAction *action) :
    ListDockWidget(main, action)
{
    setWindowTitle(tr("Exports"));
    setObjectName("ExportsWidget");

    exportsModel = new ExportsModel(&exports, this);
    exportsProxyModel = new ExportsProxyModel(exportsModel, this);
    setModels(exportsProxyModel);
    ui->treeView->sortByColumn(ExportsModel::OffsetColumn, Qt::AscendingOrder);

    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["ExportsWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [=] (){ 
            toggleDockWidget(true); 
            main->updateDockActionChecked(action);
            } );

    connect(Core(), &CutterCore::codeRebased, this, &ExportsWidget::refreshExports);
    connect(Core(), &CutterCore::refreshAll, this, &ExportsWidget::refreshExports);
}

ExportsWidget::~ExportsWidget() {}

void ExportsWidget::refreshExports()
{
    exportsModel->beginResetModel();
    exports = Core()->getAllExports();
    exportsModel->endResetModel();

    qhelpers::adjustColumns(ui->treeView, 3, 0);
}
