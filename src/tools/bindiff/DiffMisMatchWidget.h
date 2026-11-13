#ifndef DIFFMISMATCHWIDGET_H
#define DIFFMISMATCHWIDGET_H

#include "CutterDiffWindow.h"

#include <QSortFilterProxyModel>
#include <QWidget>
#include <QuickFilterView.h>

#include <AddressableItemModel.h>
#include <CutterDiff.h>
#include <CutterTreeView.h>

class DiffMismatchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit DiffMismatchProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
};

class DiffMismatchModel : public AddressableItemModel<>
{
    Q_OBJECT

    friend class CutterDiffWindow;

public:
    enum Column : ut8 {
        FuncName = 0,
        FuncAddress,
        FuncLinearSize,
        FuncNargs,
        FuncNlocals,
        FuncNbbs,
        FuncCalltype,
        FuncEdges,
        FuncStackframe,
        ColumnCount
    };

    DiffMismatchModel(QList<FunctionDescription> *list, QObject *parent = nullptr);
    ~DiffMismatchModel();
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    RVA address(const QModelIndex &index) const override;
    void reload();

private:
    QList<FunctionDescription> *list;
};

class DiffMisMatchWidget : public CutterDiffWidget
{
    Q_OBJECT
public:
    explicit DiffMisMatchWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent,
                                bool original = false);
    ~DiffMisMatchWidget() = default;

protected:
    void reload() override;
signals:

private:
    const bool original;
    CutterTreeView *treeView;
    DiffMismatchModel *model;
    QList<FunctionDescription> list;
    DiffMismatchProxyModel *proxyModel;
    QuickFilterView *mismatchesFilter;
};

#endif // DIFFMISMATCHWIDGET_H
