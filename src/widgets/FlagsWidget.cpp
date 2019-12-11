#include "FlagsWidget.h"
#include "ui_FlagsWidget.h"
#include "core/MainWindow.h"
#include "dialogs/RenameDialog.h"
#include "common/Helpers.h"

#include <QComboBox>
#include <QMenu>
#include <QShortcut>
#include <QTreeWidget>

FlagsModel::FlagsModel(QList<FlagDescription> *flags, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent),
      flags(flags)
{
}

int FlagsModel::rowCount(const QModelIndex &) const
{
    return flags->count();
}

int FlagsModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant FlagsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= flags->count())
        return QVariant();

    const FlagDescription &flag = flags->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SIZE:
            return RSizeString(flag.size);
        case OFFSET:
            return RAddressString(flag.offset);
        case NAME:
            return flag.name;
        default:
            return QVariant();
        }
    case FlagDescriptionRole:
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
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA FlagsModel::address(const QModelIndex &index) const
{
   const FlagDescription &flag = flags->at(index.row());
   return flag.offset;
}

QString FlagsModel::name(const QModelIndex &index) const
{
    const FlagDescription &flag = flags->at(index.row());
    return flag.name;
}

FlagsSortFilterProxyModel::FlagsSortFilterProxyModel(FlagsModel *source_model, QObject *parent)
    : AddressableFilterProxyModel(source_model, parent)
{
}

bool FlagsSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    FlagDescription flag = index.data(FlagsModel::FlagDescriptionRole).value<FlagDescription>();
    return flag.name.contains(filterRegExp());
}

bool FlagsSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    FlagDescription left_flag = left.data(FlagsModel::FlagDescriptionRole).value<FlagDescription>();
    FlagDescription right_flag = right.data(FlagsModel::FlagDescriptionRole).value<FlagDescription>();

    switch (left.column()) {
    case FlagsModel::SIZE:
        if (left_flag.size != right_flag.size)
            return left_flag.size < right_flag.size;
    // fallthrough
    case FlagsModel::OFFSET:
        if (left_flag.offset != right_flag.offset)
            return left_flag.offset < right_flag.offset;
    // fallthrough
    case FlagsModel::NAME:
        return left_flag.name < right_flag.name;
    default:
        break;
    }

    // fallback
    return left_flag.offset < right_flag.offset;
}


FlagsWidget::FlagsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::FlagsWidget),
    main(main),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    flags_model = new FlagsModel(&flags, this);
    flags_proxy_model = new FlagsSortFilterProxyModel(flags_model, this);
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), flags_proxy_model,
            SLOT(setFilterWildcard(const QString &)));
    ui->flagsTreeView->setMainWindow(mainWindow);
    ui->flagsTreeView->setModel(flags_proxy_model);
    ui->flagsTreeView->sortByColumn(FlagsModel::OFFSET, Qt::AscendingOrder);

    // Ctrl-F to move the focus to the Filter search box
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, SIGNAL(activated()), ui->filterLineEdit, SLOT(setFocus()));
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, [this] {
        if (ui->filterLineEdit->text().isEmpty()) {
            ui->flagsTreeView->setFocus();
        } else {
            ui->filterLineEdit->setText("");
        }
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this, [this] {
        tree->showItemsNumber(flags_proxy_model->rowCount());
    });

    setScrollMode();

    connect(Core(), &CutterCore::flagsChanged, this, &FlagsWidget::flagsChanged);
    connect(Core(), &CutterCore::codeRebased, this, &FlagsWidget::flagsChanged);
    connect(Core(), &CutterCore::refreshAll, this, &FlagsWidget::refreshFlagspaces);

    auto menu = ui->flagsTreeView->getItemContextMenu();
    menu->addSeparator();
    menu->addAction(ui->actionRename);
    menu->addAction(ui->actionDelete);
    addAction(ui->actionRename);
    addAction(ui->actionDelete);
}

FlagsWidget::~FlagsWidget() {}

void FlagsWidget::on_flagspaceCombo_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);

    refreshFlags();
}

void FlagsWidget::on_actionRename_triggered()
{
    FlagDescription flag = ui->flagsTreeView->selectionModel()->currentIndex().data(
                               FlagsModel::FlagDescriptionRole).value<FlagDescription>();

    RenameDialog r(this);
    r.setName(flag.name);
    if (r.exec()) {
        QString new_name = r.getName();
        Core()->renameFlag(flag.name, new_name);
    }
}

void FlagsWidget::on_actionDelete_triggered()
{
    FlagDescription flag = ui->flagsTreeView->selectionModel()->currentIndex().data(
                               FlagsModel::FlagDescriptionRole).value<FlagDescription>();
    Core()->delFlag(flag.name);
}

void FlagsWidget::flagsChanged()
{
    refreshFlagspaces();
}

void FlagsWidget::refreshFlagspaces()
{
    int cur_idx = ui->flagspaceCombo->currentIndex();
    if (cur_idx < 0)
        cur_idx = 0;

    ui->flagspaceCombo->clear();
    ui->flagspaceCombo->addItem(tr("(all)"));

    for (const FlagspaceDescription &i : Core()->getAllFlagspaces()) {
        ui->flagspaceCombo->addItem(i.name, QVariant::fromValue(i));
    }

    if (cur_idx > 0)
        ui->flagspaceCombo->setCurrentIndex(cur_idx);

    refreshFlags();
}

void FlagsWidget::refreshFlags()
{
    QString flagspace;

    QVariant flagspace_data = ui->flagspaceCombo->currentData();
    if (flagspace_data.isValid())
        flagspace = flagspace_data.value<FlagspaceDescription>().name;


    flags_model->beginResetModel();
    flags = Core()->getAllFlags(flagspace);
    flags_model->endResetModel();

    qhelpers::adjustColumns(ui->flagsTreeView, 2, 0);

    tree->showItemsNumber(flags_proxy_model->rowCount());
    
    // TODO: this is not a very good place for the following:
    QStringList flagNames;
    for (const FlagDescription &i : flags)
        flagNames.append(i.name);
    main->refreshOmniBar(flagNames);
}

void FlagsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->flagsTreeView);
}
