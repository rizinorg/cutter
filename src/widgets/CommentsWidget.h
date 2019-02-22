#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include <memory>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"

class MainWindow;
class QTreeWidgetItem;
class CommentsWidget;

namespace Ui {
class CommentsWidget;
}

class CommentsModel : public QAbstractItemModel
{
    Q_OBJECT

    friend CommentsWidget;

private:
    QList<CommentDescription> *comments;
    QMap<QString, QList<CommentDescription>> *nestedComments;
    bool nested;

public:
    enum Column { OffsetColumn = 0, FunctionColumn, CommentColumn, ColumnCount };
    enum NestedColumn { OffsetNestedColumn = 0, CommentNestedColumn, NestedColumnCount };
    enum Role { CommentDescriptionRole = Qt::UserRole, FunctionRole };

    CommentsModel(QList<CommentDescription> *comments,
                  QMap<QString, QList<CommentDescription>> *nestedComments,
                  QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    bool isNested() const;
    void setNested(bool nested);
};

class CommentsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    CommentsProxyModel(CommentsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class CommentsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit CommentsWidget(MainWindow *main, QAction *action = nullptr);
    ~CommentsWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_commentsTreeView_doubleClicked(const QModelIndex &index);

    void on_actionHorizontal_triggered();
    void on_actionVertical_triggered();

    void showTitleContextMenu(const QPoint &pt);

    void refreshTree();

private:
    std::unique_ptr<Ui::CommentsWidget> ui;
    MainWindow *main;

    CommentsModel *commentsModel;
    CommentsProxyModel *commentsProxyModel;
    CutterTreeWidget *tree;

    QList<CommentDescription> comments;
    QMap<QString, QList<CommentDescription>> nestedComments;

    void setScrollMode();
};

#endif // COMMENTSWIDGET_H
