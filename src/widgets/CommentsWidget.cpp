#include "CommentsWidget.h"
#include "ui_ListDockWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QMenu>
#include <QResizeEvent>
#include <QShortcut>

CommentsModel::CommentsModel(QList<CommentDescription> *comments,
                             QList<CommentGroup> *nestedComments,
                             QObject *parent)
    : AddressableItemModel<>(parent),
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

RVA CommentsModel::address(const QModelIndex &index) const
{
    if (isNested()) {
        if (index.internalId() != 0) {
            auto &group = nestedComments->at(index.parent().row());
            return group.comments.at(index.row()).offset;
        } else {
            return nestedComments->at(index.row()).offset;
        }
    } else {
        return comments->at(index.row()).offset;
    }
}

QModelIndex CommentsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, (quintptr) 0);
    }

    return createIndex(row, column, (quintptr)(parent.row() + 1));
}

QModelIndex CommentsModel::parent(const QModelIndex &index) const
{
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
        return nestedComments->at(parent.row()).comments.size();
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

    QString groupName;
    CommentDescription comment;
    if (isNested()) {
        auto &group = nestedComments->at(commentIndex);
        groupName = group.name;
        if (isSubnode) {
            comment = group.comments.at(index.row());
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
                return groupName;
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
    : AddressableFilterProxyModel(sourceModel, parent)
{
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
    ListDockWidget(main, action),
    actionHorizontal(tr("Horizontal"), this),
    actionVertical(tr("Vertical"), this)
{
    setWindowTitle(tr("Comments"));
    setObjectName("CommentsWidget");

    commentsModel = new CommentsModel(&comments, &nestedComments, this);
    commentsProxyModel = new CommentsProxyModel(commentsModel, this);
    setModels(commentsProxyModel);
    ui->treeView->sortByColumn(CommentsModel::CommentColumn, Qt::AscendingOrder);

    titleContextMenu = new QMenu(this);
    auto viewTypeGroup = new QActionGroup(titleContextMenu);
    actionHorizontal.setCheckable(true);
    actionHorizontal.setActionGroup(viewTypeGroup);
    connect(&actionHorizontal, &QAction::toggled, this, &CommentsWidget::onActionHorizontalToggled);
    actionVertical.setCheckable(true);
    actionVertical.setActionGroup(viewTypeGroup);
    connect(&actionVertical, &QAction::toggled, this, &CommentsWidget::onActionVerticalToggled);
    titleContextMenu->addActions(viewTypeGroup->actions());


    actionHorizontal.setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, &CommentsWidget::showTitleContextMenu);

    connect(Core(), &CutterCore::codeRebased, this, &CommentsWidget::refreshTree);
    connect(Core(), &CutterCore::commentsChanged, this, &CommentsWidget::refreshTree);
    connect(Core(), &CutterCore::refreshAll, this, &CommentsWidget::refreshTree);
}

CommentsWidget::~CommentsWidget() {}

void CommentsWidget::onActionHorizontalToggled(bool checked)
{
    if (checked) {
        commentsModel->setNested(false);
        ui->treeView->setIndentation(8);
    }
}

void CommentsWidget::onActionVerticalToggled(bool checked)
{
    if (checked) {
        commentsModel->setNested(true);
        ui->treeView->setIndentation(20);
    }
}

void CommentsWidget::showTitleContextMenu(const QPoint &pt)
{
    titleContextMenu->exec(this->mapToGlobal(pt));
}

void CommentsWidget::resizeEvent(QResizeEvent *event)
{
    if (mainWindow->responsive && isVisible()) {
        if (event->size().width() >= event->size().height()) {
            // Set horizontal view (list)
            actionHorizontal.setChecked(true);
        } else {
            // Set vertical view (Tree)
            actionVertical.setChecked(true);
        }
    }
    QDockWidget::resizeEvent(event);
}

void CommentsWidget::refreshTree()
{
    commentsModel->beginResetModel();

    comments = Core()->getAllComments("CCu");
    nestedComments.clear();
    QMap<QString, size_t> nestedCommentMapping;
    for (const CommentDescription &comment : comments) {
        RVA offset = RVA_INVALID;
        QString fcnName = Core()->nearestFlag(comment.offset, &offset);
        auto nestedCommentIt = nestedCommentMapping.find(fcnName);
        if (nestedCommentIt == nestedCommentMapping.end()) {
            nestedCommentMapping.insert(fcnName, nestedComments.size());
            nestedComments.push_back({fcnName, offset, {comment}});
        } else {
            auto &commentGroup = nestedComments[nestedCommentIt.value()];
            commentGroup.comments.append(comment);
        }
    }

    commentsModel->endResetModel();

    qhelpers::adjustColumns(ui->treeView, 3, 0);
}

