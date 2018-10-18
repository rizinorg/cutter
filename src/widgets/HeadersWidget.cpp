#include "HeadersWidget.h"
#include "ui_HeadersWidget.h"
#include "MainWindow.h"
#include "common/Helpers.h"

HeadersModel::HeadersModel(QList<HeaderDescription> *headers, QObject *parent)
    : QAbstractListModel(parent),
      headers(headers)
{
}

int HeadersModel::rowCount(const QModelIndex &) const
{
    return headers->count();
}

int HeadersModel::columnCount(const QModelIndex &) const
{
    return HeadersModel::ColumnCount;
}

QVariant HeadersModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= headers->count())
        return QVariant();

    const HeaderDescription &header = headers->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OffsetColumn:
            return RAddressString(header.vaddr);
        case NameColumn:
            return header.name;
        case ValueColumn:
            return header.value;
        default:
            return QVariant();
        }
    case HeaderDescriptionRole:
        return QVariant::fromValue(header);
    default:
        return QVariant();
    }
}

QVariant HeadersModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OffsetColumn:
            return tr("Offset");
        case NameColumn:
            return tr("Name");
        case ValueColumn:
            return tr("Value");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

HeadersProxyModel::HeadersProxyModel(HeadersModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool HeadersProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    HeaderDescription item = index.data(HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();
    return item.name.contains(filterRegExp());
}

bool HeadersProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    HeaderDescription leftHeader = left.data(
                                       HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();
    HeaderDescription rightHeader = right.data(
                                        HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();

    switch (left.column()) {
    case HeadersModel::OffsetColumn:
        return leftHeader.vaddr < rightHeader.vaddr;
    case HeadersModel::NameColumn:
        return leftHeader.name < rightHeader.name;
    case HeadersModel::ValueColumn:
        return leftHeader.value < rightHeader.value;
    default:
        break;
    }

    return leftHeader.vaddr < rightHeader.vaddr;
}

HeadersWidget::HeadersWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::HeadersWidget)
{
    ui->setupUi(this);

    headersModel = new HeadersModel(&headers, this);
    headersProxyModel = new HeadersProxyModel(headersModel, this);
    ui->headersTreeView->setModel(headersProxyModel);
    ui->headersTreeView->sortByColumn(HeadersModel::OffsetColumn, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), &CutterCore::refreshAll, this, &HeadersWidget::refreshHeaders);
}

HeadersWidget::~HeadersWidget() {}

void HeadersWidget::refreshHeaders()
{
    headersModel->beginResetModel();
    headers = Core()->getAllHeaders();
    headersModel->endResetModel();

    ui->headersTreeView->resizeColumnToContents(0);
    ui->headersTreeView->resizeColumnToContents(1);
}

void HeadersWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->headersTreeView);
}

void HeadersWidget::on_headersTreeView_doubleClicked(const QModelIndex &index)
{
    HeaderDescription item = index.data(HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();
    Core()->seek(item.vaddr);
}
