#include "HeapDockWidget.h"
#include "ui_HeapDockWidget.h"
#include <Cutter.h>

HeapDockWidget::HeapDockWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::HeapDockWidget)
{
    ui->setupUi(this);
    ui->allocatorSelector->addItem("Glibc Heap");
    connect<void (QComboBox::*)(int)>(ui->allocatorSelector, &QComboBox::currentIndexChanged, this,
                                      &HeapDockWidget::onAllocatorSelected);
}

HeapDockWidget::~HeapDockWidget()
{
    delete ui;
}
void HeapDockWidget::onAllocatorSelected(int index)
{
    if (index == Glibc) {
    }
}
