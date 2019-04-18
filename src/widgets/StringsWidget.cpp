#include "StringsWidget.h"
#include "ui_StringsWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "dialogs/XrefsDialog.h"
#include "WidgetShortcuts.h"

#include <QClipboard>
#include <QMenu>
#include <QModelIndex>
#include <QShortcut>

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
    return StringsModel::ColumnCount;
}

QVariant StringsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= strings->count())
        return QVariant();

    const StringDescription &str = strings->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case StringsModel::OffsetColumn:
            return RAddressString(str.vaddr);
        case StringsModel::StringColumn:
            return str.string;
        case StringsModel::TypeColumn:
            return str.type.toUpper();
        case StringsModel::LengthColumn:
            return str.length;
        case StringsModel::SizeColumn:
            return str.size;
        case StringsModel::SectionColumn:
            return str.section;
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
        case StringsModel::OffsetColumn:
            return tr("Address");
        case StringsModel::StringColumn:
            return tr("String");
        case StringsModel::TypeColumn:
            return tr("Type");
        case StringsModel::LengthColumn:
            return tr("Length");
        case StringsModel::SizeColumn:
            return tr("Size");
        case StringsModel::SectionColumn:
            return tr("Section");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

StringsProxyModel::StringsProxyModel(StringsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool StringsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    StringDescription str = index.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    if (selectedSection.isEmpty())
        return str.string.contains(filterRegExp());
    else
        return selectedSection == str.section && str.string.contains(filterRegExp());
}

bool StringsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftStr = left.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    auto rightStr = right.data(StringsModel::StringDescriptionRole).value<StringDescription>();

    switch (left.column()) {
    case StringsModel::OffsetColumn:
        return leftStr.vaddr < rightStr.vaddr;
    case StringsModel::StringColumn: // sort by string
        return leftStr.string < rightStr.string;
    case StringsModel::TypeColumn: // sort by type
        return leftStr.type < rightStr.type;
    case StringsModel::SizeColumn: // sort by size
        return leftStr.size < rightStr.size;
    case StringsModel::LengthColumn: // sort by length
        return leftStr.length < rightStr.length;
    case StringsModel::SectionColumn:
        return leftStr.section < rightStr.section;
    default:
        break;
    }

    // fallback
    return leftStr.vaddr < rightStr.vaddr;
}


StringsWidget::StringsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::StringsWidget),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);
    ui->quickFilterView->setLabelText(tr("Section:"));

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    qhelpers::setVerticalScrollMode(ui->stringsTreeView);

    // Shift-F12 to toggle strings window
    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["StringsWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [ = ] () {
        toggleDockWidget(true);
        main->updateDockActionChecked(action);
    } );

    connect(ui->actionCopy_String, SIGNAL(triggered()), this, SLOT(on_actionCopy()));
    connect(ui->actionCopy_Address, SIGNAL(triggered()), this, SLOT(on_actionCopy()));

    connect(ui->stringsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showStringsContextMenu(const QPoint &)));

    ui->actionFilter->setShortcut(QKeySequence::Find);

    ui->stringsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    model = new StringsModel(&strings, this);
    proxyModel = new StringsProxyModel(model, this);
    ui->stringsTreeView->setModel(proxyModel);
    ui->stringsTreeView->sortByColumn(StringsModel::OffsetColumn, Qt::AscendingOrder);

    auto xRefShortcut = new QShortcut(QKeySequence{Qt::CTRL + Qt::Key_X}, this);
    xRefShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    ui->actionX_refs->setShortcut(Qt::CTRL + Qt::Key_X);
    connect(xRefShortcut, SIGNAL(activated()), this, SLOT(on_actionX_refs_triggered()));

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)), proxyModel,
            SLOT(setFilterWildcard(const QString &)));

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(proxyModel->rowCount());
    });

    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &ComboQuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &ComboQuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshStrings()));

    connect(
        ui->quickFilterView->comboBox(), &QComboBox::currentTextChanged, this,
        [this]() {
            proxyModel->selectedSection = ui->quickFilterView->comboBox()->currentData().toString();
            proxyModel->setFilterRegExp(proxyModel->filterRegExp());
            tree->showItemsNumber(proxyModel->rowCount());
        }
    );
}

StringsWidget::~StringsWidget() {}

void StringsWidget::on_stringsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    StringDescription str = index.data(StringsModel::StringDescriptionRole).value<StringDescription>();
    Core()->seek(str.vaddr);
}

void StringsWidget::refreshStrings()
{
    if (task) {
        task->wait();
    }

    task = QSharedPointer<StringsTask>(new StringsTask());
    connect(task.data(), &StringsTask::stringSearchFinished, this,
            &StringsWidget::stringSearchFinished);
    Core()->getAsyncTaskManager()->start(task);

    refreshSectionCombo();
}

void StringsWidget::refreshSectionCombo()
{
    QComboBox *combo = ui->quickFilterView->comboBox();

    combo->clear();
    combo->addItem(tr("(all)"));

    for (const QString &section : Core()->getSectionList()) {
        combo->addItem(section, section);
    }

    proxyModel->selectedSection.clear();
}

void StringsWidget::stringSearchFinished(const QList<StringDescription> &strings)
{
    model->beginResetModel();
    this->strings = strings;
    model->endResetModel();

    qhelpers::adjustColumns(ui->stringsTreeView, 5, 0);
    if (ui->stringsTreeView->columnWidth(1) > 300)
        ui->stringsTreeView->setColumnWidth(1, 300);

    tree->showItemsNumber(proxyModel->rowCount());

    task = nullptr;
}

void StringsWidget::showStringsContextMenu(const QPoint &pt)
{
    QMenu *menu = new QMenu(ui->stringsTreeView);

    menu->clear();
    menu->addAction(ui->actionCopy_String);
    menu->addAction(ui->actionCopy_Address);
    menu->addAction(ui->actionFilter);
    menu->addSeparator();
    menu->addAction(ui->actionX_refs);

    menu->exec(ui->stringsTreeView->mapToGlobal(pt));

    delete menu;
}

void StringsWidget::on_actionX_refs_triggered()
{
    StringDescription str = ui->stringsTreeView->selectionModel()->currentIndex().data(
                                StringsModel::StringDescriptionRole).value<StringDescription>();

    XrefsDialog x(nullptr);
    x.fillRefsForAddress(str.vaddr, RAddressString(str.vaddr), false);
    x.exec();
}

void StringsWidget::on_actionCopy()
{
    QModelIndex current_item = ui->stringsTreeView->currentIndex();
    int row = current_item.row();

    QModelIndex index;

    if (sender() == ui->actionCopy_String) {
        index = ui->stringsTreeView->model()->index(row, 1);
    } else if (sender() == ui->actionCopy_Address) {
        index = ui->stringsTreeView->model()->index(row, 0);
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(index.data().toString());
}
