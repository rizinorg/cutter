#include "StringsWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_StringsWidget.h"

#include <QClipboard>
#include <QMenu>
#include <QModelIndex>
#include <QShortcut>

#include <utility>

StringsModel::StringsModel(QObject *parent) : AddressableItemModel<QAbstractListModel>(parent) {}

int StringsModel::rowCount(const QModelIndex &) const
{
    return strings.count();
}

int StringsModel::columnCount(const QModelIndex &) const
{
    return StringsModel::ColumnCount;
}

QVariant StringsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= strings.count()) {
        return QVariant();
    }

    const StringDescription &str = strings.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case StringsModel::OffsetColumn:
            return rzAddressString(str.vaddr);
        case StringsModel::StringColumn:
            return str.string;
        case StringsModel::TypeColumn:
            return str.type.toUpper();
        case StringsModel::LengthColumn:
            return QString::number(str.length);
        case StringsModel::SizeColumn:
            return QString::number(str.size);
        case StringsModel::SectionColumn:
            return str.section;
        case StringsModel::CommentColumn:
            return Core()->getCommentAt(str.vaddr);
        default:
            return QVariant();
        }
    case stringDescriptionRole:
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
        case StringsModel::CommentColumn:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA StringsModel::address(const QModelIndex &index) const
{
    const StringDescription &str = strings.at(index.row());
    return str.vaddr;
}

const StringDescription *StringsModel::description(const QModelIndex &index) const
{
    return &strings.at(index.row());
}

StringsProxyModel::StringsProxyModel(StringsModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void StringsProxyModel::setSelectedSection(QString section)
{

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    beginFilterChange();
#endif

    selectedSection = std::move(section);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    invalidateFilter();
#elif QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    invalidateRowsFilter();
#else
    endFilterChange();
#endif
}

bool StringsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto str = index.data(StringsModel::stringDescriptionRole).value<StringDescription>();
    if (selectedSection.isEmpty()) {
        return qhelpers::filterStringContains(str.string, this);
    } else {
        return selectedSection == str.section && qhelpers::filterStringContains(str.string, this);
    }
}

bool StringsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto model = static_cast<StringsModel *>(sourceModel());
    auto leftStr = model->description(left);
    auto rightStr = model->description(right);

    switch (left.column()) {
    case StringsModel::OffsetColumn:
        return leftStr->vaddr < rightStr->vaddr;
    case StringsModel::StringColumn: // sort by string
        return leftStr->string < rightStr->string;
    case StringsModel::TypeColumn: // sort by type
        return leftStr->type < rightStr->type;
    case StringsModel::SizeColumn: // sort by size
        return leftStr->size < rightStr->size;
    case StringsModel::LengthColumn: // sort by length
        return leftStr->length < rightStr->length;
    case StringsModel::SectionColumn:
        return leftStr->section < rightStr->section;
    case StringsModel::CommentColumn:
        return Core()->getCommentAt(leftStr->vaddr) < Core()->getCommentAt(rightStr->vaddr);
    default:
        break;
    }

    // fallback
    return leftStr->vaddr < rightStr->vaddr;
}

StringsWidget::StringsWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::StringsWidget),
      model(new StringsModel(this)),
      proxyModel(new StringsProxyModel(model, this))
{
    ui->setupUi(this);
    ui->quickFilterView->setLabelText(tr("Section:"));

    qhelpers::setVerticalScrollMode(ui->stringsTreeView);

    // Shift-F12 to toggle strings window
    const QShortcut *toggleShortcut = Shortcuts()->makeQShortcut("Strings.toggle", main);
    connect(toggleShortcut, &QShortcut::activated, this, [=, this]() { toggleDockWidget(true); });

    connect(ui->actionCopyString, &QAction::triggered, this, &StringsWidget::onActionCopy);

    ui->actionFilter->setShortcut(QKeySequence::Find);

    ui->stringsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->stringsTreeView->setMainWindow(main);
    ui->stringsTreeView->setModel(static_cast<AddressableItemModelI *>(proxyModel));
    ui->stringsTreeView->sortByColumn(-1, Qt::AscendingOrder);

    ui->rawStringsCheckBox->setChecked(Config()->getShowRawStrings());
    ui->rawStringsCheckBox->setStyleSheet("QCheckBox {"
                                          "    padding-left: 10px;"
                                          "    padding-right: 10px;"
                                          "    padding-top: 5px;"
                                          "    padding-bottom: 0px;"
                                          "}");

    auto menu = ui->stringsTreeView->getItemContextMenu();
    menu->addAction(ui->actionCopyString);

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, proxyModel,
            &QSortFilterProxyModel::setFilterWildcard);

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, this,
            [this] { ui->quickFilterView->setItemCount(proxyModel->rowCount()); });

    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &ComboQuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, this, [this]() {
        ui->quickFilterView->clearFilter();
        ui->stringsTreeView->setFocus();
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(Core(), &CutterCore::refreshAll, this, &StringsWidget::refreshStrings);
    connect(Core(), &CutterCore::codeRebased, this, &StringsWidget::refreshStrings);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(model, StringsModel::CommentColumn); });

    connect(ui->quickFilterView->comboBox(), &QComboBox::currentTextChanged, this, [this]() {
        proxyModel->setSelectedSection(ui->quickFilterView->comboBox()->currentData().toString());
        ui->quickFilterView->setItemCount(proxyModel->rowCount());
    });

    auto showRawStringsChecked = [this](bool checked) {
        Config()->setShowRawStrings(checked);
        refreshStrings();
    };
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    connect(ui->rawStringsCheckBox, &QCheckBox::checkStateChanged, this, showRawStringsChecked);
#else
    connect(ui->rawStringsCheckBox, &QCheckBox::stateChanged, this, showRawStringsChecked);
#endif
}

StringsWidget::~StringsWidget() {}

void StringsWidget::refreshStrings()
{
    if (task) {
        task->wait();
    }

    task = std::shared_ptr<StringsTask>(new StringsTask(ui->rawStringsCheckBox->isChecked()));
    connect(task.get(), &StringsTask::stringSearchFinished, this,
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

    proxyModel->setSelectedSection(QString());
}

void StringsWidget::stringSearchFinished(const QList<StringDescription> &strings)
{
    model->beginResetModel();
    model->strings = strings;
    model->endResetModel();

    qhelpers::adjustColumns(ui->stringsTreeView, StringsModel::ColumnCount, 0);

    // set the initial item count
    ui->quickFilterView->setItemCount(proxyModel->rowCount());

    task.reset();
}

void StringsWidget::onActionCopy()
{
    const QModelIndex currentItem = ui->stringsTreeView->currentIndex();
    const int row = currentItem.row();

    QModelIndex index;

    index = ui->stringsTreeView->model()->index(row, 1);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(index.data().toString());
}
