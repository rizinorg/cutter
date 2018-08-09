
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

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OFFSET:
            return RAddressString(str.vaddr);
        case STRING:
            return str.string;
        case TYPE:
            return str.type.toUpper();
        case LENGTH:
            return str.length;
        case SIZE:
            return str.size;
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
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OFFSET:
            return tr("Address");
        case STRING:
            return tr("String");
        case TYPE:
            return tr("Type");
        case LENGTH:
            return tr("Length");
        case SIZE:
            return tr("Size");
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

StringsSortFilterProxyModel::StringsSortFilterProxyModel(StringsModel *source_model,
                                                         QObject *parent)
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
    StringDescription left_str = left.data(
                                     StringsModel::StringDescriptionRole).value<StringDescription>();
    StringDescription right_str = right.data(
                                      StringsModel::StringDescriptionRole).value<StringDescription>();

    switch (left.column()) {
    case StringsModel::OFFSET:
        return left_str.vaddr < right_str.vaddr;
    case StringsModel::STRING: // sort by string
        return left_str.string < right_str.string;
    case StringsModel::TYPE: // sort by type
        return left_str.type < right_str.type;
    case StringsModel::SIZE: // sort by size
        return left_str.size < right_str.size;
    case StringsModel::LENGTH: // sort by length
        return left_str.length < right_str.length;
    default:
        break;
    }

    // fallback
    return left_str.vaddr < right_str.vaddr;
}


StringsWidget::StringsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
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

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)), proxy_model,
            SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->stringsTreeView, SLOT(setFocus()));

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshStrings()));
}

StringsWidget::~StringsWidget() {}

void StringsWidget::on_stringsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    StringDescription str = index.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    Core()->seek(str.vaddr);
}

void StringsWidget::refreshStrings()
{
    if (task) {
        task->wait();
    }

    task = QSharedPointer<StringsTask>(new StringsTask());
    connect(task.data(), &StringsTask::stringSearchFinished, this, &StringsWidget::stringSearchFinished);
    Core()->getAsyncTaskManager()->start(task);
}

void StringsWidget::stringSearchFinished(const QList<StringDescription> &strings)
{
    model->beginReload();
    this->strings = strings;
    model->endReload();

    qhelpers::adjustColumns(ui->stringsTreeView, 5, 0);
    if (ui->stringsTreeView->columnWidth(1) > 300)
        ui->stringsTreeView->setColumnWidth(1, 300);

    task = nullptr;
}
