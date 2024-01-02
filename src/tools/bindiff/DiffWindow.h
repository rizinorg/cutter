#ifndef DIFF_WINDOW_H
#define DIFF_WINDOW_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

#include <core/Cutter.h>

class DiffWindow;

namespace Ui {
class DiffWindow;
}

class DiffMatchModel : public QAbstractListModel
{
    Q_OBJECT

    friend DiffWindow;

public:
    enum Column {
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

    friend DiffWindow;

public:
    enum Column {
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

class DiffWindow : public QDialog
{
    Q_OBJECT

public:
    explicit DiffWindow(BinDiff *bd, QWidget *parent = nullptr);
    ~DiffWindow();

private slots:
    void actionExport_as_JSON();
    void actionExport_as_Markdown();

private:
    std::unique_ptr<Ui::DiffWindow> ui;
    std::unique_ptr<BinDiff> bDiff;

    DiffMatchModel *modelMatch;
    DiffMismatchModel *modelDel;
    DiffMismatchModel *modelAdd;

    QList<BinDiffMatchDescription> listMatch;
    QList<FunctionDescription> listDel;
    QList<FunctionDescription> listAdd;
};

#endif // DIFF_WINDOW_H
