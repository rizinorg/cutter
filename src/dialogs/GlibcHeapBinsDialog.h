#ifndef GLIBCHEAPBINSDIALOG_H
#define GLIBCHEAPBINSDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include "core/Cutter.h"
#include <MainWindow.h>
#include <HeapBinsGraphView.h>

namespace Ui {
class GlibcHeapBinsDialog;
}

class BinsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit BinsModel(RVA arena_addr, QObject *parent = nullptr);
    enum Column {
        TypeColumn = 0,
        BinNumColumn,
        FdColumn,
        BkColumn,
        CountColumn,
        SizeColumn,
        ColumnCount
    };
    void reload();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    void clearData();
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    RVA arena_addr = 0;
    RzList *getChunks(int index);
    QString getBinMessage(int index);
    QVector<RzHeapBin *> values;

private:
};

class GlibcHeapBinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GlibcHeapBinsDialog(RVA i, MainWindow *main, QWidget *parent);
    ~GlibcHeapBinsDialog();
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &prev);
    void setChainInfo(int index);
    void setGraphView(int index);
private slots:
    void showHeapInfoDialog();

private:
    Ui::GlibcHeapBinsDialog *ui;
    RVA m_state;
    BinsModel *binsModel {};
    HeapBinsGraphView *graphView;
    MainWindow *main;
};

#endif // GLIBCHEAPBINSDIALOG_H
