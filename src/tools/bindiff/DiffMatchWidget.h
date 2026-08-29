#ifndef DIFFMATCHWIDGET_H
#define DIFFMATCHWIDGET_H

#include "CutterDiffWindow.h"

#include <QAction>
#include <QWidget>

#include <Configuration.h>
#include <CutterDiff.h>
#include <CutterTreeView.h>

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
    void reload();
signals:
private:
    // CutterDiff *cutterDiff;
    // CutterDiffWindow *diffWindow;
    CutterTreeView *treeView;
    DiffMatchModel *model;
    QList<BinDiffMatchDescription> list;

private:
    void showContextMenu(const QPoint &pos);
};

#endif // DIFFMATCHWIDGET_H
