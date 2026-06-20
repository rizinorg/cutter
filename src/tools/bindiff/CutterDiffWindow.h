#ifndef CUTTERDIFFWINDOW_H
#define CUTTERDIFFWINDOW_H

#include <QAction>
#include <QMainWindow>

#include <AddressableItemModel.h>
#include <CutterTreeView.h>
#include <core/BinDiff.h>
#include <core/CutterDiff.h>

namespace Ui {
class CutterDiffWindow;
}

class CutterDiffWindow;

class DiffMatchModel : public QAbstractListModel
{
    Q_OBJECT

    friend CutterDiffWindow;

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

    DiffMatchModel(QList<BinDiffMatchDescription> *list, QColor cPerf, QColor cPart,
                   QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QColor gradientByRatio(const double ratio) const;

private:
    QList<BinDiffMatchDescription> *list;

    QColor perfect, partial;
};

class DiffMismatchModel : public QAbstractListModel
{
    Q_OBJECT

    friend CutterDiffWindow;

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

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<FunctionDescription> *list;
};

class FunctionListModel : public AddressableItemModel<>
{
    Q_OBJECT
public:
    FunctionListModel(QList<FunctionDescription> *list, QObject *parent = nullptr);
    ~FunctionListModel() {}
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
    void refreshModel();

private:
    QList<FunctionDescription> *list;
};

class CutterDiffWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CutterDiffWindow(QWidget *parent = nullptr);
    ~CutterDiffWindow();

public slots:
    void onBinDiffCompleted();
    void onActionDiffNewFile();

private:
    Ui::CutterDiffWindow *ui;
    CutterDiff *cutterDiff;
    BinDiff *bDiff;
    DiffMatchModel *matches;
    DiffMismatchModel *added;
    DiffMismatchModel *removed;
    QList<BinDiffMatchDescription> listMatch;
    QList<FunctionDescription> listDel;
    QList<FunctionDescription> listAdd;
    FunctionListModel *modelA;
    FunctionListModel *modelB;
    QList<FunctionDescription> fcnsA;
    QList<FunctionDescription> fcnsB;
    void setupFonts();
    void refreshHex(RVA addr);
};

#endif // CUTTERDIFFWINDOW_H
