#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include <memory>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "common/AddressableItemModel.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"
#include "widgets/ListDockWidget.h"

class MainWindow;
class QTreeWidgetItem;
class CommentsWidget;

struct CommentGroup
{
    QString name;
    RVA offset;
    QList<CommentDescription> comments;
};

class CommentsModel : public AddressableItemModel<>
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CommentsModel)                                                          \
        CommentsModel(const CommentsModel &m) = delete;                                            \
        CommentsModel &operator=(const CommentsModel &m) = delete;

#    define Q_DISABLE_MOVE(CommentsModel)                                                          \
        CommentsModel(CommentsModel &&w) = delete;                                                 \
        CommentsModel &operator=(CommentsModel &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(CommentsModel)                                                     \
        Q_DISABLE_COPY(CommentsModel)                                                              \
        Q_DISABLE_MOVE(CommentsModel)
#endif

    Q_DISABLE_COPY_MOVE(CommentsModel)

    friend CommentsWidget;

private:
    QList<CommentDescription> *comments;
    QList<CommentGroup> *nestedComments;
    bool nested;

public:
    enum Column { OffsetColumn = 0, FunctionColumn, CommentColumn, ColumnCount };
    enum NestedColumn { OffsetNestedColumn = 0, CommentNestedColumn, NestedColumnCount };
    enum Role { CommentDescriptionRole = Qt::UserRole, FunctionRole };

    CommentsModel(QList<CommentDescription> *comments, QList<CommentGroup> *nestedComments,
                  QObject *parent = nullptr);
    ~CommentsModel() override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    bool isNested() const;
    void setNested(bool nested);

    RVA address(const QModelIndex &index) const override;
};

class CommentsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CommentsProxyModel)                                                     \
        CommentsProxyModel(const CommentsProxyModel &d) = delete;                                  \
        CommentsProxyModel &operator=(const CommentsProxyModel &d) = delete;

#    define Q_DISABLE_MOVE(CommentsProxyModel)                                                     \
        CommentsProxyModel(CommentsProxyModel &&d) = delete;                                       \
        CommentsProxyModel &operator=(CommentsProxyModel &&d) = delete;

#    define Q_DISABLE_COPY_MOVE(CommentsProxyModel)                                                \
        Q_DISABLE_COPY(CommentsProxyModel)                                                         \
        Q_DISABLE_MOVE(CommentsProxyModel)
#endif

    Q_DISABLE_COPY_MOVE(CommentsProxyModel)

public:
    CommentsProxyModel(CommentsModel *sourceModel, QObject *parent = nullptr);
    ~CommentsProxyModel() override;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class CommentsWidget : public ListDockWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CommentsWidget)                                                         \
        CommentsWidget(const CommentsWidget &w) = delete;                                          \
        CommentsWidget &operator=(const CommentsWidget &w) = delete;

#    define Q_DISABLE_MOVE(CommentsWidget)                                                         \
        CommentsWidget(CommentsWidget &&w) = delete;                                               \
        CommentsWidget &operator=(CommentsWidget &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(CommentsWidget)                                                    \
        Q_DISABLE_COPY(CommentsWidget)                                                             \
        Q_DISABLE_MOVE(CommentsWidget)
#endif

    Q_DISABLE_COPY_MOVE(CommentsWidget)

public:
    explicit CommentsWidget(MainWindow *main);
    ~CommentsWidget() override;

private slots:
    void onActionHorizontalToggled(bool checked);
    void onActionVerticalToggled(bool checked);

    void showTitleContextMenu(const QPoint &pt);

    void refreshTree();

private:
    CommentsModel *commentsModel;
    CommentsProxyModel *commentsProxyModel;
    QAction actionHorizontal;
    QAction actionVertical;

    QList<CommentDescription> comments;
    QList<CommentGroup> nestedComments;

    QMenu *titleContextMenu;
};

#endif // COMMENTSWIDGET_H
