#ifndef HEAPDOCKWIDGET_H
#define HEAPDOCKWIDGET_H

#include <QDockWidget>
#include "CutterDockWidget.h"

namespace Ui {
class HeapDockWidget;
}

class HeapDockWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit HeapDockWidget(MainWindow *main);
    ~HeapDockWidget();
private slots:
    void onAllocatorSelected(int index);

private:
    enum Allocator { Glibc = 0, AllocatorCount };
    Ui::HeapDockWidget *ui;
    MainWindow *main;
    QWidget* currentHeapWidget = nullptr;
};

#endif // HEAPDOCKWIDGET_H
