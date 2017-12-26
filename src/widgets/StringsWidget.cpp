
#include <QModelIndex>

#include "StringsWidget.h"
#include "ui_StringsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"


StringsModel::StringsModel(QList<StringDescription> *strings, QObject *parent)
        : QAbstractListModel(parent),
          strings(strings)
{
}

int StringsModel::rowCount(const QModelIndex &) const
{
    return strings->count();
}

int StringsModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant StringsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= strings->count())
        return QVariant();

    const StringDescription &str = strings->at(index.row());

    switch (role)
    {
        case Qt::DisplayRole:
            switch (index.column())
            {
                case OFFSET:
                    return RAddressString(str.vaddr);
                case STRING:
                    return str.string;
                default:
                    return QVariant();
            }
        case StringDescriptionRole:
            return QVariant::fromValue(str);
        default:
            return QVariant();
    }
}

QVariant StringsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
            switch (section)
            {
                case OFFSET:
                    return tr("Address");
                case STRING:
                    return tr("String");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
}

void StringsModel::beginReload()
{
    beginResetModel();
}

void StringsModel::endReload()
{
    endResetModel();
}





StringsSortFilterProxyModel::StringsSortFilterProxyModel(StringsModel *source_model, QObject *parent)
        : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool StringsSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    StringDescription str = index.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    return str.string.contains(filterRegExp());
}

bool StringsSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    StringDescription left_str = left.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    StringDescription right_str = right.data(StringsModel::StringDescriptionRole).value<StringDescription>();

    switch (left.column())
    {
        case StringsModel::OFFSET:
            if (left_str.vaddr != right_str.vaddr)
                return left_str.vaddr < right_str.vaddr;
            // fallthrough
        case StringsModel::STRING:
            return left_str.string < right_str.string;
        default:
            break;
    }

    // fallback
    return left_str.vaddr < right_str.vaddr;
}


StringsWidget::StringsWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::StringsWidget)
{
    ui->setupUi(this);

    qhelpers::setVerticalScrollMode(ui->stringsTreeView);

    // Ctrl-F to show/hide the filter entry
    QShortcut *search_shortcut = new QShortcut(QKeySequence::Find, this);
    connect(search_shortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    search_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clear_shortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    model = new StringsModel(&strings, this);
    proxy_model = new StringsSortFilterProxyModel(model, this);
    ui->stringsTreeView->setModel(proxy_model);
    ui->stringsTreeView->sortByColumn(StringsModel::OFFSET, Qt::AscendingOrder);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)), proxy_model, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->stringsTreeView, SLOT(setFocus()));

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshStrings()));
}

StringsWidget::~StringsWidget() {}

void StringsWidget::on_stringsTreeView_doubleClicked(const QModelIndex &index)
{
    StringDescription str = index.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    CutterCore::getInstance()->seek(str.vaddr);
}

void StringsWidget::refreshStrings()
{
    model->beginReload();
    strings = CutterCore::getInstance()->getAllStrings();
    model->endReload();

    ui->stringsTreeView->resizeColumnToContents(0);
    ui->stringsTreeView->resizeColumnToContents(1);
}
