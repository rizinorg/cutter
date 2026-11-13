#include "FlagsWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_FlagsWidget.h"

#include <QComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QShortcut>
#include <QStandardItemModel>
#include <QTreeWidget>

FlagsModel::FlagsModel(QObject *parent) : AddressableItemModel<QAbstractListModel>(parent) {}

int FlagsModel::rowCount(const QModelIndex &) const
{
    return flags.count();
}

int FlagsModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant FlagsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= flags.count()) {
        return QVariant();
    }

    const FlagDescription &flag = flags.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SIZE:
            return rzSizeString(flag.size);
        case OFFSET:
            return rzAddressString(flag.offset);
        case NAME:
            return flag.name;
        case REALNAME:
            return flag.realname;
        case COMMENT:
            return Core()->getCommentAt(flag.offset);
        default:
            return QVariant();
        }
    case flagDescriptionRole:
        return QVariant::fromValue(flag);
    default:
        return QVariant();
    }
}

QVariant FlagsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case SIZE:
            return tr("Size");
        case OFFSET:
            return tr("Offset");
        case NAME:
            return tr("Name");
        case REALNAME:
            return tr("Real Name");
        case COMMENT:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA FlagsModel::address(const QModelIndex &index) const
{
    const FlagDescription &flag = flags.at(index.row());
    return flag.offset;
}

QString FlagsModel::name(const QModelIndex &index) const
{
    const FlagDescription &flag = flags.at(index.row());
    return flag.name;
}

const FlagDescription *FlagsModel::description(QModelIndex index) const
{
    if (index.row() < flags.size()) {
        return &flags.at(index.row());
    }
    return nullptr;
}

FlagsSortFilterProxyModel::FlagsSortFilterProxyModel(FlagsModel *source_model, QObject *parent)
    : AddressableFilterProxyModel(source_model, parent)
{
}

bool FlagsSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto flag = index.data(FlagsModel::flagDescriptionRole).value<FlagDescription>();
    return qhelpers::filterStringContains(flag.name, this);
}

bool FlagsSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto source = static_cast<FlagsModel *>(sourceModel());
    auto leftFlag = source->description(left);
    auto rightFlag = source->description(right);

    switch (left.column()) {
    case FlagsModel::SIZE:
        if (leftFlag->size != rightFlag->size) {
            return leftFlag->size < rightFlag->size;
        }
    // fallthrough
    case FlagsModel::OFFSET:
        if (leftFlag->offset != rightFlag->offset) {
            return leftFlag->offset < rightFlag->offset;
        }
    // fallthrough
    case FlagsModel::NAME:
        return leftFlag->name < rightFlag->name;

    case FlagsModel::REALNAME:
        return leftFlag->realname < rightFlag->realname;

    case FlagsModel::COMMENT:
        return Core()->getCommentAt(leftFlag->offset) < Core()->getCommentAt(rightFlag->offset);

    default:
        break;
    }

    // fallback
    return leftFlag->offset < rightFlag->offset;
}

FlagsWidget::FlagsWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::FlagsWidget),
      main(main),
      flagsModel(new FlagsModel(this)),
      flagsProxyModel(new FlagsSortFilterProxyModel(flagsModel, this))
{
    ui->setupUi(this);

    connect(ui->filterLineEdit, &QLineEdit::textChanged, flagsProxyModel,
            &QSortFilterProxyModel::setFilterWildcard);
    ui->flagsTreeView->setMainWindow(mainWindow);
    ui->flagsTreeView->setModel(static_cast<AddressableItemModelI *>(flagsProxyModel));
    ui->flagsTreeView->sortByColumn(FlagsModel::OFFSET, Qt::AscendingOrder);

    // Ctrl-F to move the focus to the Filter search box
    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->filterLineEdit,
            [this]() { ui->filterLineEdit->setFocus(); });
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, [this] {
        if (ui->filterLineEdit->text().isEmpty()) {
            ui->flagsTreeView->setFocus();
        } else {
            ui->filterLineEdit->setText("");
        }
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this,
            [this] { ui->filterLineEdit->setItemCount(flagsProxyModel->rowCount()); });

    setScrollMode();

    connect(Core(), &CutterCore::flagsChanged, this, &FlagsWidget::flagsChanged);
    connect(Core(), &CutterCore::codeRebased, this, &FlagsWidget::flagsChanged);
    connect(Core(), &CutterCore::refreshAll, this, &FlagsWidget::refreshFlagspaces);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(flagsModel, FlagsModel::COMMENT); });

    auto menu = ui->flagsTreeView->getItemContextMenu();
    menu->addSeparator();
    menu->addAction(ui->actionRename);
    menu->addAction(ui->actionDelete);
    addAction(ui->actionRename);
    addAction(ui->actionDelete);

    connect(ui->flagspaceCombo, &QComboBox::currentTextChanged, this,
            &FlagsWidget::onFlagspaceComboCurrentTextChanged);
    connect(ui->actionRename, &QAction::triggered, this, &FlagsWidget::onActionRenameTriggered);
    connect(ui->actionDelete, &QAction::triggered, this, &FlagsWidget::onActionDeleteTriggered);
}

FlagsWidget::~FlagsWidget() {}

void FlagsWidget::onFlagspaceComboCurrentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);

    refreshFlags();
}

void FlagsWidget::onActionRenameTriggered()
{
    const auto flag = ui->flagsTreeView->selectionModel()
                              ->currentIndex()
                              .data(FlagsModel::flagDescriptionRole)
                              .value<FlagDescription>();

    bool ok;
    const QString newName =
            QInputDialog::getText(this, tr("Rename flag %1").arg(flag.name), tr("Flag name:"),
                                  QLineEdit::Normal, flag.name, &ok);
    if (ok && !newName.isEmpty()) {
        Core()->renameFlag(flag.name, newName);
    }
}

void FlagsWidget::onActionDeleteTriggered()
{
    const auto flag = ui->flagsTreeView->selectionModel()
                              ->currentIndex()
                              .data(FlagsModel::flagDescriptionRole)
                              .value<FlagDescription>();
    Core()->delFlag(flag.name);
}

void FlagsWidget::flagsChanged()
{
    refreshFlagspaces();
}

void FlagsWidget::refreshFlagspaces()
{
    int curIdx = ui->flagspaceCombo->currentIndex();
    if (curIdx < 0) {
        curIdx = 0;
    }

    disableFlagRefresh =
            true; // prevent duplicate flag refresh caused by flagspaceCombo modifications
    ui->flagspaceCombo->clear();
    ui->flagspaceCombo->addItem(tr("(all)"));

    for (const FlagspaceDescription &i : Core()->getAllFlagspaces()) {
        ui->flagspaceCombo->addItem(i.name, QVariant::fromValue(i));
    }

    if (curIdx > 0) {
        ui->flagspaceCombo->setCurrentIndex(curIdx);
    }
    disableFlagRefresh = false;

    refreshFlags();
}

void FlagsWidget::refreshFlags()
{
    if (disableFlagRefresh) {
        return;
    }
    QString flagspace;

    const QVariant flagspaceData = ui->flagspaceCombo->currentData();
    if (flagspaceData.isValid()) {
        flagspace = flagspaceData.value<FlagspaceDescription>().name;
    }

    flagsModel->beginResetModel();
    flagsModel->flags = Core()->getAllFlags(flagspace);
    flagsModel->endResetModel();

    // set the initial item count
    ui->filterLineEdit->setItemCount(flagsProxyModel->rowCount());
}

void FlagsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->flagsTreeView);
}
