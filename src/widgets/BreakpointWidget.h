#pragma once

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "AddressableItemModel.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui {
class BreakpointWidget;
}

class MainWindow;
class QTreeWidgetItem;
class BreakpointWidget;

class BreakpointModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(BreakpointModel)                                                        \
        BreakpointModel(const BreakpointModel &m) = delete;                                        \
        BreakpointModel &operator=(const BreakpointModel &m) = delete;

#    define Q_DISABLE_MOVE(BreakpointModel)                                                        \
        BreakpointModel(BreakpointModel &&m) = delete;                                             \
        BreakpointModel &operator=(BreakpointModel &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(BreakpointModel)                                                   \
        Q_DISABLE_COPY(BreakpointModel)                                                            \
        Q_DISABLE_MOVE(BreakpointModel)
#endif

    Q_DISABLE_COPY_MOVE(BreakpointModel)

    friend BreakpointWidget;

private:
    QList<BreakpointDescription> breakpoints;

public:
    enum Column {
        AddrColumn = 0,
        NameColumn,
        TypeColumn,
        TraceColumn,
        EnabledColumn,
        CommentColumn,
        ColumnCount
    };
    enum Role { BreakpointDescriptionRole = Qt::UserRole };

    BreakpointModel(QObject *parent = nullptr);
    ~BreakpointModel() override;

    void refresh();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    RVA address(const QModelIndex &index) const override;
};

class BreakpointProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(BreakpointProxyModel)                                                   \
        BreakpointProxyModel(const BreakpointProxyModel &m) = delete;                              \
        BreakpointProxyModel &operator=(const BreakpointProxyModel &m) = delete;

#    define Q_DISABLE_MOVE(BreakpointProxyModel)                                                   \
        BreakpointProxyModel(BreakpointProxyModel &&m) = delete;                                   \
        BreakpointProxyModel &operator=(BreakpointProxyModel &&m) = delete;

#    define Q_DISABLE_COPY_MOVE(BreakpointProxyModel)                                              \
        Q_DISABLE_COPY(BreakpointProxyModel)                                                       \
        Q_DISABLE_MOVE(BreakpointProxyModel)
#endif

    Q_DISABLE_COPY_MOVE(BreakpointProxyModel)

public:
    BreakpointProxyModel(BreakpointModel *sourceModel, QObject *parent = nullptr);
    ~BreakpointProxyModel() override;
};

class BreakpointWidget : public CutterDockWidget
{
    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(BreakpointWidget)                                                       \
        BreakpointWidget(const BreakpointWidget &w) = delete;                                      \
        BreakpointWidget &operator=(const BreakpointWidget &w) = delete;

#    define Q_DISABLE_MOVE(BreakpointWidget)                                                       \
        BreakpointWidget(BreakpointWidget &&w) = delete;                                           \
        BreakpointWidget &operator=(BreakpointWidget &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(BreakpointWidget)                                                  \
        Q_DISABLE_COPY(BreakpointWidget)                                                           \
        Q_DISABLE_MOVE(BreakpointWidget)
#endif

    Q_DISABLE_COPY_MOVE(BreakpointWidget)

public:
    explicit BreakpointWidget(MainWindow *main);
    ~BreakpointWidget() override;

private slots:
    void delBreakpoint();
    void toggleBreakpoint();
    void editBreakpoint();
    void addBreakpointDialog();
    void refreshBreakpoint();

private:
    std::unique_ptr<Ui::BreakpointWidget> ui;

    BreakpointModel *breakpointModel;
    BreakpointProxyModel *breakpointProxyModel;
    QList<BreakpointDescription> breakpoints;
    QAction *actionDelBreakpoint = nullptr;
    QAction *actionToggleBreakpoint = nullptr;
    QAction *actionEditBreakpoint = nullptr;

    void setScrollMode();
    QVector<RVA> getSelectedAddresses() const;

    RefreshDeferrer *refreshDeferrer;
    bool editing = false;
};
