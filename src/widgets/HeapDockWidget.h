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

private:
    Ui::HeapDockWidget *ui;
};

#endif // HEAPDOCKWIDGET_H
