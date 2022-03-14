#include "RegisterRefsWidget.h"
#include "ui_RegisterRefsWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QJsonObject>
#include <QMenu>
#include <QClipboard>
#include <QShortcut>

RegisterRefModel::RegisterRefModel(QList<RegisterRefDescription> *registerRefs, QObject *parent)
    : QAbstractListModel(parent), registerRefs(registerRefs)
{
}

int RegisterRefModel::rowCount(const QModelIndex &) const
{
    return registerRefs->count();
}

int RegisterRefModel::columnCount(const QModelIndex &) const
{
    return RegisterRefModel::ColumnCount;
}

QVariant RegisterRefModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= registerRefs->count())
        return QVariant();

    const RegisterRefDescription &registerRef = registerRefs->at(index.row());

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
    QModelIndex index = sourceModel()->index(row, 0, parent);
    RegisterRefDescription item = index.data(RegisterRefModel::RegisterRefDescriptionRole)
                                          .value<RegisterRefDescription>();
    return qhelpers::filterStringContains(item.reg, this);
}

bool RegisterRefProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    RegisterRefDescription leftRegRef =
            left.data(RegisterRefModel::RegisterRefDescriptionRole).value<RegisterRefDescription>();
    RegisterRefDescription rightRegRef = right.data(RegisterRefModel::RegisterRefDescriptionRole)
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
      tree(new CutterTreeWidget(this)),
      addressableItemContextMenu(this, main)
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    registerRefModel = new RegisterRefModel(&registerRefs, this);
    registerRefProxyModel = new RegisterRefProxyModel(registerRefModel, this);
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

    refreshDeferrer = createRefreshDeferrer([this]() { refreshRegisterRef(); });

    // Ctrl-F to show/hide the filter entry
    QShortcut *search_shortcut = new QShortcut(QKeySequence::Find, this);
    connect(search_shortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    search_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

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
            [this] { tree->showItemsNumber(registerRefProxyModel->rowCount()); });
}

RegisterRefsWidget::~RegisterRefsWidget() = default;

void RegisterRefsWidget::refreshRegisterRef()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    registerRefModel->beginResetModel();

    registerRefs.clear();
    for (const RegisterRef &reg : Core()->getRegisterRefs()) {
        RegisterRefDescription desc;

        desc.value = RzAddressString(reg.value);
        desc.reg = reg.name;
        desc.refDesc = Core()->formatRefDesc(reg.ref);

        registerRefs.push_back(desc);
    }

    registerRefModel->endResetModel();

    ui->registerRefTreeView->resizeColumnToContents(0);
    ui->registerRefTreeView->resizeColumnToContents(1);
    ui->registerRefTreeView->resizeColumnToContents(2);

    tree->showItemsNumber(registerRefProxyModel->rowCount());
}

void RegisterRefsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->registerRefTreeView);
}

void RegisterRefsWidget::on_registerRefTreeView_doubleClicked(const QModelIndex &index)
{
    RegisterRefDescription item = index.data(RegisterRefModel::RegisterRefDescriptionRole)
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

    RVA offset = Core()->math(offsetString);
    addressableItemContextMenu.setTarget(offset);
}

void RegisterRefsWidget::copyClip(int column)
{
    int row = ui->registerRefTreeView->selectionModel()->currentIndex().row();
    QString value = ui->registerRefTreeView->selectionModel()
                            ->currentIndex()
                            .sibling(row, column)
                            .data()
                            .toString();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(value);
}
