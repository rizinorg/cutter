#ifndef REGISTERREFSWIDGET_H
#define REGISTERREFSWIDGET_H

#include "CutterDockWidget.h"
#include "core/Cutter.h"
#include "menus/AddressableItemContextMenu.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <memory>

class MainWindow;
class QTreeWidget;
class RegisterRefsWidget;

namespace Ui {
class RegisterRefsWidget;
}

class MainWindow;
class QTreeWidgetItem;

struct RegisterRefDescription
{
    QString reg;
    QString value;
    RefDescription refDesc;
};
Q_DECLARE_METATYPE(RegisterRefDescription)

/**
 * @brief Source model for @ref RegisterRefModel
 */
class RegisterRefModel : public QAbstractListModel
{
    Q_OBJECT

    friend RegisterRefsWidget;

private:
    QList<RegisterRefDescription> registerRefs;

public:
    enum Column : ut8 { RegColumn = 0, ValueColumn, RefColumn, CommentColumn, ColumnCount };
    enum Role : ut16 { RegisterRefDescriptionRole = Qt::UserRole };

    RegisterRefModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

/**
 * @brief Sort and Filter proxy model for @ref RegisterRefModel
 */
class RegisterRefProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    RegisterRefProxyModel(RegisterRefModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget that lists register values, updates them as the debugger runs, and allows users to
 * "seek" (navigate) to referenced memory
 */
class RegisterRefsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RegisterRefsWidget(MainWindow *main);
    ~RegisterRefsWidget();

private slots:
    void onRegisterRefTreeViewDoubleClicked(const QModelIndex &index);
    void refreshRegisterRef();
    void copyClip(int column);
    void customMenuRequested(QPoint pos);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    std::unique_ptr<Ui::RegisterRefsWidget> ui;

    RegisterRefModel *registerRefModel;
    RegisterRefProxyModel *registerRefProxyModel;
    void setScrollMode();

    RefreshDeferrer *refreshDeferrer;

    QAction *actionCopyValue;
    QAction *actionCopyRef;
    AddressableItemContextMenu addressableItemContextMenu;
};

#endif // REGISTERREFSWIDGET_H
