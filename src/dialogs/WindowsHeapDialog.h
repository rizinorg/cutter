#ifndef WINDOWSHEAPDIALOG_H
#define WINDOWSHEAPDIALOG_H

#include <QDialog>
#include <ui_WindowsHeapDialog.h>
#include <CutterDescriptions.h>

namespace Ui {
class WindowsHeapDialog;
}

class HeapInfoModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit HeapInfoModel(QObject *parent = nullptr);
    enum Column { BaseColumn = 0, AllocatedColumn, CommittedColumn, BlockCountColumn, ColumnCount };
    void reload();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<WindowsHeapInfo> values;
};

class WindowsHeapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WindowsHeapDialog(QWidget *parent);
    ~WindowsHeapDialog() override;
private slots:
    void updateContents();

private:
    Ui::WindowsHeapDialog *ui;
    QTableView *viewHeap;
    HeapInfoModel *modelHeap = new HeapInfoModel(this);
};
#endif // WINDOWSHEAPDIALOG_H
