#include "HeadersWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_ListDockWidget.h"

HeadersModel::HeadersModel(QObject *parent) : AddressableItemModel<QAbstractListModel>(parent) {}

int HeadersModel::rowCount(const QModelIndex &) const
{
    return headers.count();
}

int HeadersModel::columnCount(const QModelIndex &) const
{
    return HeadersModel::ColumnCount;
}

QVariant HeadersModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= headers.count()) {
        return QVariant();
    }

    const HeaderDescription &header = headers.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OffsetColumn:
            return rzAddressString(header.vaddr);
        case NameColumn:
            return header.name;
        case ValueColumn:
            return header.value;
        case CommentColumn:
            return Core()->getCommentAt(header.vaddr);
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
        case CommentColumn:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA HeadersModel::address(const QModelIndex &index) const
{
    const HeaderDescription &header = headers.at(index.row());
    return header.vaddr;
}

QString HeadersModel::name(const QModelIndex &index) const
{
    const HeaderDescription &header = headers.at(index.row());
    return header.name;
}

HeadersProxyModel::HeadersProxyModel(HeadersModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
}

bool HeadersProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto item = index.data(HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();
    return qhelpers::filterStringContains(item.name, this);
}

bool HeadersProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftHeader =
            left.data(HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();
    const auto rightHeader =
            right.data(HeadersModel::HeaderDescriptionRole).value<HeaderDescription>();

    switch (left.column()) {
    case HeadersModel::OffsetColumn:
        return leftHeader.vaddr < rightHeader.vaddr;
    case HeadersModel::NameColumn:
        return leftHeader.name < rightHeader.name;
    case HeadersModel::ValueColumn:
        return leftHeader.value < rightHeader.value;
    case HeadersModel::CommentColumn:
        return Core()->getCommentAt(leftHeader.vaddr) < Core()->getCommentAt(rightHeader.vaddr);
    default:
        break;
    }

    return leftHeader.vaddr < rightHeader.vaddr;
}

HeadersWidget::HeadersWidget(MainWindow *main)
    : ListDockWidget(main),
      headersModel(new HeadersModel(this)),
      headersProxyModel(new HeadersProxyModel(headersModel, this))
{
    setWindowTitle(tr("Headers"));
    setObjectName("HeadersWidget");

    setModels(headersProxyModel);
    ui->treeView->sortByColumn(HeadersModel::OffsetColumn, Qt::AscendingOrder);

    connect(Core(), &CutterCore::codeRebased, this, &HeadersWidget::refreshHeaders);
    connect(Core(), &CutterCore::refreshAll, this, &HeadersWidget::refreshHeaders);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(headersModel, HeadersModel::CommentColumn); });
    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this,
            [this] { ui->quickFilterView->setItemCount(headersProxyModel->rowCount()); });
}

HeadersWidget::~HeadersWidget() {}

void HeadersWidget::refreshHeaders()
{
    headersModel->beginResetModel();
    headersModel->headers = Core()->getAllHeaders();
    headersModel->endResetModel();

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);

    // set the initial item count
    ui->quickFilterView->setItemCount(headersProxyModel->rowCount());
}
