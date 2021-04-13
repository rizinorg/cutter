#include "ZignaturesWidget.h"
#include "ui_ZignaturesWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

ZignaturesModel::ZignaturesModel(QList<ZignatureDescription> *zignatures, QObject *parent)
    : QAbstractListModel(parent), zignatures(zignatures)
{
}

int ZignaturesModel::rowCount(const QModelIndex &) const
{
    return zignatures->count();
}

int ZignaturesModel::columnCount(const QModelIndex &) const
{
    return ZignaturesModel::ColumnCount;
}

QVariant ZignaturesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= zignatures->count())
        return QVariant();

    const ZignatureDescription &zignature = zignatures->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OffsetColumn:
            return RAddressString(zignature.offset);
        case NameColumn:
            return zignature.name;
        case ValueColumn:
            return zignature.bytes;
        default:
            return QVariant();
        }

    case ZignatureDescriptionRole:
        return QVariant::fromValue(zignature);

    case Qt::ToolTipRole: {
        QString tmp = QString("Graph:\n\n    Cyclomatic complexity: " + RSizeString(zignature.cc)
                              + "\n    Nodes / basicblocks: " + RSizeString(zignature.nbbs)
                              + "\n    Edges: " + RSizeString(zignature.edges)
                              + "\n    Ebbs: " + RSizeString(zignature.ebbs) + "\n\nRefs:\n");
        for (const QString &ref : zignature.refs) {
            tmp.append("\n    " + ref);
        }
        return tmp;
    }

    default:
        return QVariant();
    }
}

QVariant ZignaturesModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OffsetColumn:
            return tr("Offset");
        case NameColumn:
            return tr("Name");
        case ValueColumn:
            return tr("Bytes");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

ZignaturesProxyModel::ZignaturesProxyModel(ZignaturesModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool ZignaturesProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    ZignatureDescription item =
            index.data(ZignaturesModel::ZignatureDescriptionRole).value<ZignatureDescription>();
    return qhelpers::filterStringContains(item.name, this);
}

bool ZignaturesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ZignatureDescription leftZignature =
            left.data(ZignaturesModel::ZignatureDescriptionRole).value<ZignatureDescription>();
    ZignatureDescription rightZignature =
            right.data(ZignaturesModel::ZignatureDescriptionRole).value<ZignatureDescription>();

    switch (left.column()) {
    case ZignaturesModel::OffsetColumn:
        return leftZignature.offset < rightZignature.offset;
    case ZignaturesModel::NameColumn:
        return leftZignature.name < rightZignature.name;
    case ZignaturesModel::ValueColumn:
        return leftZignature.bytes < rightZignature.bytes;
    default:
        break;
    }

    return leftZignature.offset < rightZignature.offset;
}

ZignaturesWidget::ZignaturesWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::ZignaturesWidget)
{
    ui->setupUi(this);

    zignaturesModel = new ZignaturesModel(&zignatures, this);
    zignaturesProxyModel = new ZignaturesProxyModel(zignaturesModel, this);
    ui->zignaturesTreeView->setModel(zignaturesProxyModel);
    ui->zignaturesTreeView->sortByColumn(ZignaturesModel::OffsetColumn, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), &CutterCore::refreshAll, this, &ZignaturesWidget::refreshZignatures);
}

ZignaturesWidget::~ZignaturesWidget() {}

void ZignaturesWidget::refreshZignatures()
{
    zignaturesModel->beginResetModel();
    zignatures = Core()->getAllZignatures();
    zignaturesModel->endResetModel();

    ui->zignaturesTreeView->resizeColumnToContents(0);
    ui->zignaturesTreeView->resizeColumnToContents(1);
    ui->zignaturesTreeView->resizeColumnToContents(2);
}

void ZignaturesWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->zignaturesTreeView);
}

void ZignaturesWidget::on_zignaturesTreeView_doubleClicked(const QModelIndex &index)
{
    ZignatureDescription item =
            index.data(ZignaturesModel::ZignatureDescriptionRole).value<ZignatureDescription>();
    Core()->seekAndShow(item.offset);
}
