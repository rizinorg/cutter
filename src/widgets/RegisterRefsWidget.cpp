#include "RegisterRefsWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_RegisterRefsWidget.h"

#include <QClipboard>
#include <QJsonObject>
#include <QMenu>
#include <QShortcut>

RegisterRefModel::RegisterRefModel(QObject *parent) : QAbstractListModel(parent) {}

int RegisterRefModel::rowCount(const QModelIndex &) const
{
    return registerRefs.count();
}

int RegisterRefModel::columnCount(const QModelIndex &) const
{
    return RegisterRefModel::ColumnCount;
}

QVariant RegisterRefModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= registerRefs.count()) {
        return QVariant();
    }

    const RegisterRefDescription &registerRef = registerRefs.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case RegColumn:
            return registerRef.reg;
        case ValueColumn:
            return registerRef.value;
        case RefColumn:
            return registerRef.refDesc.ref;
        case CommentColumn:
            return Core()->getCommentAt(Core()->math(registerRef.value));
        default:
            return QVariant();
        }
    case Qt::ForegroundRole:
        switch (index.column()) {
        case RefColumn:
            return registerRef.refDesc.refColor;
        default:
            return QVariant();
        }
    case RegisterRefDescriptionRole:
        return QVariant::fromValue(registerRef);
    default:
        return QVariant();
    }
}

QVariant RegisterRefModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case RegColumn:
            return tr("Register");
        case ValueColumn:
            return tr("Value");
        case RefColumn:
            return tr("Reference");
        case CommentColumn:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RegisterRefProxyModel::RegisterRefProxyModel(RegisterRefModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool RegisterRefProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto item = index.data(RegisterRefModel::RegisterRefDescriptionRole)
                              .value<RegisterRefDescription>();
    return qhelpers::filterStringContains(item.reg, this);
}

bool RegisterRefProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftRegRef =
            left.data(RegisterRefModel::RegisterRefDescriptionRole).value<RegisterRefDescription>();
    const auto rightRegRef = right.data(RegisterRefModel::RegisterRefDescriptionRole)
                                     .value<RegisterRefDescription>();

    switch (left.column()) {
    case RegisterRefModel::RegColumn:
        return leftRegRef.reg < rightRegRef.reg;
    case RegisterRefModel::RefColumn:
        return leftRegRef.refDesc.ref < rightRegRef.refDesc.ref;
    case RegisterRefModel::ValueColumn:
        return leftRegRef.value < rightRegRef.value;
    case RegisterRefModel::CommentColumn:
        return Core()->getCommentAt(Core()->math(leftRegRef.value))
                < Core()->getCommentAt(Core()->math(rightRegRef.value));
    default:
        break;
    }

    return leftRegRef.reg < rightRegRef.reg;
}

RegisterRefsWidget::RegisterRefsWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::RegisterRefsWidget),
      registerRefModel(new RegisterRefModel(this)),
      registerRefProxyModel(new RegisterRefProxyModel(registerRefModel, this)),
      refreshDeferrer(createRefreshDeferrer([this]() { refreshRegisterRef(); })),
      addressableItemContextMenu(this, main)
{
    ui->setupUi(this);

    ui->registerRefTreeView->setModel(registerRefProxyModel);
    ui->registerRefTreeView->setAutoScroll(false);
    ui->registerRefTreeView->sortByColumn(RegisterRefModel::RegColumn, Qt::AscendingOrder);

    actionCopyValue = new QAction(tr("Copy register value"), this);
    actionCopyRef = new QAction(tr("Copy register reference"), this);

    addressableItemContextMenu.addAction(actionCopyValue);
    addressableItemContextMenu.addAction(actionCopyRef);
    addActions(addressableItemContextMenu.actions());

    connect(ui->registerRefTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &RegisterRefsWidget::onCurrentChanged);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, registerRefProxyModel,
            &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->quickFilterView, &QuickFilterView::filterClosed, ui->registerRefTreeView,
            [this]() { ui->registerRefTreeView->setFocus(); });
    setScrollMode();
    connect(Core(), &CutterCore::refreshAll, this, &RegisterRefsWidget::refreshRegisterRef);
    connect(Core(), &CutterCore::registersChanged, this, &RegisterRefsWidget::refreshRegisterRef);
    connect(Core(), &CutterCore::commentsChanged, this, [this]() {
        qhelpers::emitColumnChanged(registerRefModel, RegisterRefModel::CommentColumn);
    });
    connect(actionCopyValue, &QAction::triggered, this,
            [this]() { copyClip(RegisterRefModel::ValueColumn); });
    connect(actionCopyRef, &QAction::triggered, this,
            [this]() { copyClip(RegisterRefModel::RefColumn); });
    ui->registerRefTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->registerRefTreeView, &QMenu::customContextMenuRequested, this,
            &RegisterRefsWidget::customMenuRequested);

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this,
            [this] { ui->quickFilterView->setItemCount(registerRefProxyModel->rowCount()); });

    connect(ui->registerRefTreeView, &QTreeView::doubleClicked, this,
            &RegisterRefsWidget::onRegisterRefTreeViewDoubleClicked);
}

RegisterRefsWidget::~RegisterRefsWidget() = default;

void RegisterRefsWidget::refreshRegisterRef()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    registerRefModel->beginResetModel();

    registerRefModel->registerRefs.clear();
    for (const RegisterRef &reg : Core()->getRegisterRefs()) {
        RegisterRefDescription desc;

        desc.value = rzAddressString(reg.value);
        desc.reg = reg.name;
        desc.refDesc = Core()->formatRefDesc(std::make_shared<AddrRefs>(reg.ref));

        registerRefModel->registerRefs.push_back(desc);
    }

    registerRefModel->endResetModel();

    ui->registerRefTreeView->resizeColumnToContents(0);
    ui->registerRefTreeView->resizeColumnToContents(1);
    ui->registerRefTreeView->resizeColumnToContents(2);

    // set the initial item count
    ui->quickFilterView->setItemCount(registerRefProxyModel->rowCount());
}

void RegisterRefsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->registerRefTreeView);
}

void RegisterRefsWidget::onRegisterRefTreeViewDoubleClicked(const QModelIndex &index)
{
    const auto item = index.data(RegisterRefModel::RegisterRefDescriptionRole)
                              .value<RegisterRefDescription>();
    Core()->seekAndShow(item.value);
}

void RegisterRefsWidget::customMenuRequested(QPoint pos)
{
    addressableItemContextMenu.exec(ui->registerRefTreeView->viewport()->mapToGlobal(pos));
}

void RegisterRefsWidget::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    auto currentIndex = ui->registerRefTreeView->selectionModel()->currentIndex();

    // Use the value column as the offset
    QString offsetString;
    if (currentIndex.column() != RegisterRefModel::RefColumn) {
        offsetString = currentIndex.data().toString();
    } else {
        offsetString = currentIndex.sibling(currentIndex.row(), RegisterRefModel::ValueColumn)
                               .data()
                               .toString();
    }

    const RVA offset = Core()->math(offsetString);
    addressableItemContextMenu.setTarget(offset);
}

void RegisterRefsWidget::copyClip(int column)
{
    const int row = ui->registerRefTreeView->selectionModel()->currentIndex().row();
    const QString value = ui->registerRefTreeView->selectionModel()
                                  ->currentIndex()
                                  .sibling(row, column)
                                  .data()
                                  .toString();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(value);
}
