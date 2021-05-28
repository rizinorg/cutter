#ifndef HEAPWIDGET_H
#define HEAPWIDGET_H

#include <QDockWidget>
#include "CutterDockWidget.h"
#include "core/Cutter.h"
#include <QTableView>

namespace Ui {
class HeapWidget;
}

class HeapModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct Chunk
    {
        RVA offset;
        QString status;
        int size;
    };
    HeapModel(QObject *parent = nullptr);
    enum Column { OffsetColumn = 0, SizeColumn, StatusColumn, ColumnCount };
    void reload();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

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

private:
    Ui::HeapWidget *ui;
    QTableView *viewHeap = new QTableView(this);
    HeapModel *modelHeap = new HeapModel(this);
};

#endif // HEAPWIDGET_H
