#include "ImportsWidget.h"
#include "ui_ImportsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTreeWidget>
#include <QPen>
#include <QPainter>

ImportsModel::ImportsModel(QList<ImportDescription> *imports, QObject *parent) :
    QAbstractTableModel(parent),
    imports(imports)
{}

int ImportsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid()? 0 : imports->count();
}

int ImportsModel::columnCount(const QModelIndex&) const
{
    return COUNT;
}

QVariant ImportsModel::data(const QModelIndex &index, int role) const
{
    const ImportDescription &import = imports->at(index.row());
    switch(role)
    {
    case AddressRole:
        return import.plt;
    case Qt::ForegroundRole:
        if(index.column() < COUNT)
            if(banned.match(import.name).hasMatch())
                return QColor(255, 129, 123);
        break;
    case Qt::DisplayRole:
        switch(index.column())
        {
        case ADDRESS:
            return RAddressString(import.plt);
        case TYPE:
            return import.type;
        case SAFETY:
            return banned.match(import.name).hasMatch()? tr("Unsafe") : QStringLiteral("");
        case NAME:
            return import.name;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return QVariant();
}

QVariant ImportsModel::headerData(int section, Qt::Orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        switch(section)
        {
        case ADDRESS:
            return tr("Address");
        case TYPE:
            return tr("Type");
        case SAFETY:
            return tr("Safety");
        case NAME:
            return tr("Name");
        default:
            break;
        }
    }
    return QVariant();
}

void ImportsModel::beginReload()
{
    beginResetModel();
}

void ImportsModel::endReload()
{
    endResetModel();
}

void CMyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);

    itemOption.rect.adjust(10, 0, 0,
                           0);  // Make the item rectangle 10 pixels smaller from the left side.

    // Draw your item content.
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter, nullptr);

    // And now you can draw a bottom border.
    //painter->setPen(Qt::cyan);
    QPen pen = painter->pen();
    pen.setColor(Qt::white);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(itemOption.rect.bottomLeft(), itemOption.rect.bottomRight());
}

/*
 * Imports Widget
 */

ImportsWidget::ImportsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ImportsWidget),
    model(new ImportsModel(&imports, this))
{
    ui->setupUi(this);

    ui->importsTreeView->setModel(model);
    ui->importsTreeView->sortByColumn(3, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshImports()));
}

ImportsWidget::~ImportsWidget() {}

void ImportsWidget::refreshImports()
{
    model->beginReload();
    imports = Core()->getAllImports();
    model->endReload();
    qhelpers::adjustColumns(ui->importsTreeView, 4, 0);
}

void ImportsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->importsTreeView);
}

void ImportsWidget::on_importsTreeView_doubleClicked(const QModelIndex &index)
{
    Core()->seek(index.data(ImportsModel::AddressRole).toLongLong());
}
