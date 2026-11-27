#ifndef DIFFMATCHWIDGET_H
#define DIFFMATCHWIDGET_H

#include "CutterDiffWindow.h"

#include <QAction>
#include <QCheckBox>
#include <QSortFilterProxyModel>
#include <QWidget>
#include <QuickFilterView.h>

#include <Configuration.h>
#include <CutterDiff.h>
#include <CutterTreeView.h>

class DiffMatchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit DiffMatchProxyModel(QObject *parent = nullptr);

    void setHidePerfectMatches(bool hide);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
    bool hidePerfectMatches = false;
};

class DiffMatchModel : public QAbstractListModel
{
    Q_OBJECT

    friend class CutterDiffWindow;

public:
    enum Column : ut8 {
        NameOrig = 0,
        SizeOrig,
        AddressOrig,
        Similarity,
        AddressMod,
        SizeMod,
        NameMod,
        ColumnCount
    };

    DiffMatchModel(QList<BinDiffMatchDescription> *list, QObject *parent = nullptr);
    ~DiffMatchModel();
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QColor gradientByRatio(const double ratio) const;
    QPair<RVA, RVA> address(const QModelIndex &index) const;
    void reload();

private:
    QList<BinDiffMatchDescription> *list;

    QColor perfect, partial;
};

class DiffMatchWidget : public CutterDiffWidget
{
    Q_OBJECT
public:
    explicit DiffMatchWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent);
    ~DiffMatchWidget() = default;

protected:
    void reload() override;
signals:
private:
    // CutterDiff *cutterDiff;
    // CutterDiffWindow *diffWindow;
    CutterTreeView *treeView;
    DiffMatchModel *model;
    DiffMatchProxyModel *proxyModel;
    QList<BinDiffMatchDescription> list;
    QCheckBox *checkBoxHideIdentical;
    QuickFilterView *matchesFilter;

private:
};

#endif // DIFFMATCHWIDGET_H
