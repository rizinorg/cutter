#include <QMenu>
#include <QResizeEvent>

#include "CommentsWidget.h"
#include "ui_CommentsWidget.h"
#include "MainWindow.h"
#include "common/Helpers.h"

CommentsModel::CommentsModel(QList<CommentDescription> *comments,
                             QMap<QString, QList<CommentDescription> > *nestedComments,
                             QObject *parent)
    : QAbstractItemModel(parent),
      comments(comments),
      nestedComments(nestedComments),
      nested(false)
{}

bool CommentsModel::isNested() const
{
    return nested;
}

void CommentsModel::setNested(bool nested)
{
    beginResetModel();
    this->nested = nested;
    endResetModel();
}

QModelIndex CommentsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, (quintptr) 0);
    }

    return createIndex(row, column, (quintptr)(parent.row() + 1));
}

QModelIndex CommentsModel::parent(const QModelIndex &index) const {
    /* Ignore invalid indexes and root nodes */
    if (!index.isValid() || index.internalId() == 0) {
        return QModelIndex();
    }

    return this->index((int)(index.internalId() - 1), 0);
}

int CommentsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return (isNested() ? nestedComments->size() : comments->count());

    if (isNested() && parent.internalId() == 0) {
        QString fcnName = nestedComments->keys().at(parent.row());
        return nestedComments->operator[](fcnName).size();
    }

    return 0;
}

int CommentsModel::columnCount(const QModelIndex &) const
{
    return (isNested()
            ? static_cast<int>(CommentsModel::NestedColumnCount)
            : static_cast<int>(CommentsModel::ColumnCount));
}

QVariant CommentsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.internalId() != 0 && !index.parent().isValid()))
        return QVariant();

    int commentIndex;
    bool isSubnode;
    if (index.internalId() != 0) {
        /* Subnode */
        commentIndex = index.parent().row();
        isSubnode = true;
    } else {
        /* Root node */
        commentIndex = index.row();
        isSubnode = false;
    }

    QString offset;
    CommentDescription comment;
    if (isNested()) {
        offset = nestedComments->keys().at(commentIndex);
        if (isSubnode) {
            comment = nestedComments->operator[](offset).at(index.row());
        }
    } else {
        comment = comments->at(commentIndex);
    }

    switch (role) {
    case Qt::DisplayRole:
        if (isNested()) {
            if (isSubnode) {
                switch (index.column()) {
                case OffsetNestedColumn:
                    return RAddressString(comment.offset);
                case CommentNestedColumn:
                    return comment.name;
                default:
                    break;
                }
            } else if (index.column() == OffsetNestedColumn) {
                return offset;
            }
        } else {
            switch (index.column()) {
            case CommentsModel::OffsetColumn:
                return RAddressString(comment.offset);
            case CommentsModel::FunctionColumn:
                return Core()->cmdFunctionAt(comment.offset);
            case CommentsModel::CommentColumn:
                return comment.name;
            default:
                break;
            }
        }
        break;
    case CommentsModel::CommentDescriptionRole:
        if (isNested() && index.internalId() == 0) {
            break;
        }
        return QVariant::fromValue(comment);
    default:
        break;
    }

    return QVariant();
}

QVariant CommentsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (isNested()) {
            switch (section) {
            case CommentsModel::OffsetNestedColumn:
                return tr("Function/Offset");
            case CommentsModel::CommentNestedColumn:
                return tr("Comment");
            default:
                break;
            }
        } else {
            switch (section) {
            case CommentsModel::OffsetColumn:
                return tr("Offset");
            case CommentsModel::FunctionColumn:
                return tr("Function");
            case CommentsModel::CommentColumn:
                return tr("Comment");
            default:
                break;
            }
        }
    }

    return QVariant();
}

CommentsProxyModel::CommentsProxyModel(CommentsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool CommentsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    CommentsModel *srcModel = static_cast<CommentsModel *>(sourceModel());
    if (srcModel->isNested()) {
        // Disable filtering
        return true;
    }

    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto comment = index.data(CommentsModel::CommentDescriptionRole).value<CommentDescription>();

    return comment.name.contains(filterRegExp());
}

bool CommentsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    CommentsModel *srcModel = static_cast<CommentsModel *>(sourceModel());
    if (srcModel->isNested()) {
        // Disable sorting
        return false;
    }

    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

    auto leftComment = left.data(CommentsModel::CommentDescriptionRole).value<CommentDescription>();
    auto rightComment = right.data(CommentsModel::CommentDescriptionRole).value<CommentDescription>();

    switch (left.column()) {
    case CommentsModel::OffsetColumn:
        return leftComment.offset < rightComment.offset;
    case CommentsModel::FunctionColumn:
        return Core()->cmdFunctionAt(leftComment.offset) < Core()->cmdFunctionAt(rightComment.offset);
    case CommentsModel::CommentColumn:
        return leftComment.name < rightComment.name;
    default:
        break;
    }

    return false;
}

CommentsWidget::CommentsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::CommentsWidget),
    main(main),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    commentsModel = new CommentsModel(&comments, &nestedComments, this);
    commentsProxyModel = new CommentsProxyModel(commentsModel, this);
    ui->commentsTreeView->setModel(commentsProxyModel);
    ui->commentsTreeView->sortByColumn(CommentsModel::CommentColumn, Qt::AscendingOrder);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)),
            commentsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->commentsTreeView, SLOT(setFocus()));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(commentsProxyModel->rowCount());
    });
    
    setScrollMode();

    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshTree()));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTree()));
}

CommentsWidget::~CommentsWidget() {}

void CommentsWidget::on_commentsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (commentsModel->isNested() && !index.parent().isValid())
        return;

    auto comment = index.data(CommentsModel::CommentDescriptionRole).value<CommentDescription>();
    Core()->seek(comment.offset);
}

void CommentsWidget::on_actionHorizontal_triggered()
{
    commentsModel->setNested(false);
    ui->commentsTreeView->setIndentation(8);
}

void CommentsWidget::on_actionVertical_triggered()
{
    commentsModel->setNested(true);
    ui->commentsTreeView->setIndentation(20);
}

void CommentsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (!commentsModel->isNested()) {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    } else {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void CommentsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible()) {
        if (event->size().width() >= event->size().height()) {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        } else {
            // Set vertical view (Tree)
            on_actionVertical_triggered();
        }
    }
    QDockWidget::resizeEvent(event);
}

void CommentsWidget::refreshTree()
{
    commentsModel->beginResetModel();

    comments = Core()->getAllComments("CCu");
    nestedComments.clear();
    for (const CommentDescription &comment : comments) {
        QString fcnName = Core()->cmdFunctionAt(comment.offset);
        nestedComments[fcnName].append(comment);
    }

    commentsModel->endResetModel();

    qhelpers::adjustColumns(ui->commentsTreeView, 3, 0);

    tree->showItemsNumber(commentsProxyModel->rowCount());
}

void CommentsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->commentsTreeView);
}
