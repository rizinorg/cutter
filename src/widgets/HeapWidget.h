#ifndef HEAPWIDGET_H
#define HEAPWIDGET_H

#include <QDockWidget>
#include "CutterDockWidget.h"
#include "core/Cutter.h"
#include <QTableView>
#include <QComboBox>
#include <AddressableItemContextMenu.h>

namespace Ui {
class HeapWidget;
}

class HeapModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit HeapModel(QObject *parent = nullptr);
    enum Column { OffsetColumn = 0, SizeColumn, StatusColumn, ColumnCount };
    void reload();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    RVA arena_addr = 0;

private:
    QVector<Chunk> values;
};

class HeapWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit HeapWidget(MainWindow *main);
    ~HeapWidget();
private slots:
    void updateContents();
    void onDoubleClicked(const QModelIndex &index);
    void onArenaSelected(int index);
    void customMenuRequested(QPoint pos);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void updateArenas();
    void updateChunks();
    Ui::HeapWidget *ui;
    QTableView *viewHeap = new QTableView(this);
    QComboBox *arenaSelectorView = new QComboBox(this);
    HeapModel *modelHeap = new HeapModel(this);
    QVector<Arena> arenas;
    AddressableItemContextMenu addressableItemContextMenu;
};

#endif // HEAPWIDGET_H
