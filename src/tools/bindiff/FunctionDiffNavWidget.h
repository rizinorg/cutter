#ifndef FUNCTIONDIFFNAVWIDGET_H
#define FUNCTIONDIFFNAVWIDGET_H

#include "CutterDiffWindow.h"

#include <QLabel>
#include <QSortFilterProxyModel>
#include <QWidget>
#include <QuickFilterView.h>

#include <AddressableItemModel.h>
#include <CutterDiff.h>
#include <CutterTreeView.h>

class FunctionListProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FunctionListProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
};

class FunctionListModel : public AddressableItemModel<>
{
    Q_OBJECT
public:
    FunctionListModel(QList<FunctionDescription> *list, QObject *parent = nullptr);
    ~FunctionListModel();
    enum Column : ut8 { Name, Offset, ColumnCount };
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    RVA address(const QModelIndex &index) const override;
    void reload();

private:
    QList<FunctionDescription> *list;
};

class FunctionDiffNavWidget : public CutterDiffWidget
{
    Q_OBJECT
public:
    explicit FunctionDiffNavWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent);
    ~FunctionDiffNavWidget() = default;

protected:
    void reload() override;

private:
    QLabel *labelFileA;
    QLabel *labelFileB;
    CutterTreeView *treeViewA;
    CutterTreeView *treeViewB;
    QList<FunctionDescription> fcnsA;
    QList<FunctionDescription> fcnsB;
    FunctionListModel *modelA;
    FunctionListModel *modelB;
    FunctionListProxyModel *proxyModelA;
    FunctionListProxyModel *proxyModelB;
    QuickFilterView *filterA;
    QuickFilterView *filterB;
};

#endif // FUNCTIONDIFFNAVWIDGET_H
