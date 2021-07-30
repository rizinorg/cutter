#ifndef WINDOWSHEAPWIDGET_H
#define WINDOWSHEAPWIDGET_H

#include <QWidget>
#include <QAbstractTableModel>
#include <MainWindow.h>
#include <QTableView>

namespace Ui {
class WindowsHeapWidget;
}

class WindowsHeapModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit WindowsHeapModel(QObject *parent = nullptr);
    enum Column {
        HeaderAddColumn = 0,
        UserAddColumn,
        SizeColumn,
        GranularityColumn,
        UnusedColumn,
        TypeColumn,
        ColumnCount
    };
    void reload();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<HeapBlock> values;
};

class WindowsHeapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WindowsHeapWidget(MainWindow *main, QWidget *parent);
    ~WindowsHeapWidget();
private slots:
    void updateContents();
    void viewHeapInfo();

private:
    Ui::WindowsHeapWidget *ui;
    QTableView *viewHeap;
    WindowsHeapModel *modelHeap = new WindowsHeapModel(this);
    RefreshDeferrer *refreshDeferrer {};
};

#endif // WINDOWSHEAPWIDGET_H
