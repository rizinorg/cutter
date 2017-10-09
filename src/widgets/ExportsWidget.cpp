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
    return Columns::COUNT;
}

QVariant ExportsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= exports->count())
        return QVariant();

    const ExportDescription &exp = exports->at(index.row());

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case OFFSET:
            return RAddressString(exp.vaddr);
        case SIZE:
            return RSizeString(exp.size);
        case TYPE:
            return exp.type;
        case NAME:
            return exp.name;
        default:
            return QVariant();
        }
    case ExportDescriptionRole:
        return QVariant::fromValue(exp);
    default:
        return QVariant();
    }
}

QVariant ExportsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        switch (section)
        {
        case OFFSET:
            return tr("Address");
        case SIZE:
            return tr("Size");
        case TYPE:
            return tr("Type");
        case NAME:
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





ExportsSortFilterProxyModel::ExportsSortFilterProxyModel(ExportsModel *source_model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
}

bool ExportsSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    ExportDescription exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    return exp.name.contains(filterRegExp());
}

bool ExportsSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ExportDescription left_exp = left.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    ExportDescription right_exp = right.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();

    switch (left.column())
    {
    case ExportsModel::SIZE:
        if (left_exp.size != right_exp.size)
            return left_exp.size < right_exp.size;
    // fallthrough
    case ExportsModel::OFFSET:
        if (left_exp.vaddr != right_exp.vaddr)
            return left_exp.vaddr < right_exp.vaddr;
    // fallthrough
    case ExportsModel::NAME:
        return left_exp.name < right_exp.name;
    case ExportsModel::TYPE:
        if (left_exp.type != right_exp.type)
            return left_exp.type < right_exp.type;
    default:
        break;
    }

    // fallback
    return left_exp.vaddr < right_exp.vaddr;
}



ExportsWidget::ExportsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::ExportsWidget),
    main(main)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    exports_model = new ExportsModel(&exports, this);
    exports_proxy_model = new ExportsSortFilterProxyModel(exports_model, this);
    ui->exportsTreeView->setModel(exports_proxy_model);
    ui->exportsTreeView->sortByColumn(ExportsModel::OFFSET, Qt::AscendingOrder);
}

ExportsWidget::~ExportsWidget() {}

void ExportsWidget::setup()
{
    setScrollMode();

    refreshExports();
}

void ExportsWidget::refresh()
{
    setup();
}

void ExportsWidget::refreshExports()
{
    exports_model->beginReloadExports();
    exports = CutterCore::getInstance()->getAllExports();
    exports_model->endReloadExports();

    ui->exportsTreeView->resizeColumnToContents(0);
    ui->exportsTreeView->resizeColumnToContents(1);
    ui->exportsTreeView->resizeColumnToContents(2);
}


void ExportsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->exportsTreeView);
}

void ExportsWidget::on_exportsTreeView_doubleClicked(const QModelIndex &index)
{
    ExportDescription exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    CutterCore::getInstance()->seek(exp.vaddr);
}
