#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "common/AddressableItemModel.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

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

    friend CommentsWidget;

private:
    QList<CommentDescription> comments;
    QList<CommentGroup> nestedComments;
    bool nested;

public:
    enum Column : ut8 { OffsetColumn = 0, FunctionColumn, CommentColumn, ColumnCount };
    enum NestedColumn : ut8 { OffsetNestedColumn = 0, CommentNestedColumn, NestedColumnCount };
    enum Role : ut16 { CommentDescriptionRole = Qt::UserRole, FunctionRole };

    CommentsModel(QObject *parent = nullptr);

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

public:
    CommentsProxyModel(CommentsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class CommentsWidget : public ListDockWidget
{
    Q_OBJECT

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

    QMenu *titleContextMenu;
};

#endif // COMMENTSWIDGET_H
