#include "HeapDockWidget.h"
#include "ui_HeapDockWidget.h"

HeapDockWidget::HeapDockWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::HeapDockWidget)
{
    ui->setupUi(this);

}

HeapDockWidget::~HeapDockWidget()
{
    delete ui;
}
