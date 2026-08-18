#ifndef CUTTERDIFFWINDOW_H
#define CUTTERDIFFWINDOW_H

#include "GraphDiffWidget.h"
#include "HexDiff.h"
#include "LineDiffWidget.h"

#include <QAction>
#include <QMainWindow>
#include <QSyntaxHighlighter>

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
    ~DiffMatchModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QColor gradientByRatio(const double ratio) const;

    QPair<RVA, RVA> address(const QModelIndex &index) const;

    void diffNew(const QString &fileA, const QString &fileB, int compareLogic, int analysisLevel);
    void clearDiff();

private:
    QList<BinDiffMatchDescription> *list;

    QColor perfect, partial;
};

class DiffMismatchModel : public AddressableItemModel<>
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

private:
    QList<FunctionDescription> *list;
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
    void refreshModel();
    bool highLightAddress();

private:
    QList<FunctionDescription> *list;
};

class CutterDiffWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CutterDiffWindow(std::unique_ptr<BinDiff> bDiff, QWidget *parent = nullptr);
    ~CutterDiffWindow();
    void seekAndShowHexDiff(QPair<RVA, RVA> addr);
public slots:
    // void onBinDiffCompleted();
    void onActionDiffNewFile();
private slots:
    void onCopyMD5AClicked();
    void onCopyShA1AClicked();
    void onCopyShA256AClicked();
    void onCopyCrC32AClicked();
    void onCopyMD5BClicked();
    void onCopyShA1BClicked();
    void onCopyShA256BClicked();
    void onCopyCrC32BClicked();
    void selectionChanged(HexDiff::Selection selection);
    void showContextMenuMatches(const QPoint &pos);
    void selectFunction(const QModelIndex &index);
    void showDiff();

private:
    Ui::CutterDiffWindow *ui;
    CutterDiff *cutterDiff;
    // BinDiff thread which fetches basic analysis results and processing
    std::unique_ptr<BinDiff> bDiff;
    // model for matched functions of both binaries
    DiffMatchModel *matches;
    // model for added functions
    DiffMismatchModel *added;
    // model for removed functions
    DiffMismatchModel *removed;
    // match results
    QList<BinDiffMatchDescription> listMatch;
    // added function descriptions
    QList<FunctionDescription> listDel;
    // removed function descriptions
    QList<FunctionDescription> listAdd;
    // model a functions
    FunctionListModel *modelA;
    // model b functions
    FunctionListModel *modelB;
    QList<FunctionDescription> fcnsA;
    QList<FunctionDescription> fcnsB;
    HexDiff *hexDiff = nullptr;
    LineDiffWidget *lineDiff = nullptr;
    GraphDiffWidget *graphDiff = nullptr;
    void addHexDiff();
    void addLineDiff();
    void addGraphDiff();
    void setupFonts();
    void clearParseWindow();
    void updateParseWindow(HexDiff::Selection selection);
    void exportDiff();
};

#endif // CUTTERDIFFWINDOW_H
