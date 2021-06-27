#include "HeapDockWidget.h"
#include "ui_HeapDockWidget.h"
#include "widgets/GlibcHeapWidget.h"

HeapDockWidget::HeapDockWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::HeapDockWidget), main(main)
{
    ui->setupUi(this);
    ui->allocatorSelector->addItem("Glibc Heap");
    connect<void (QComboBox::*)(int)>(ui->allocatorSelector, &QComboBox::currentIndexChanged, this,
                                      &HeapDockWidget::onAllocatorSelected);
    ui->verticalLayout->setMargin(0);
    onAllocatorSelected(0);
}

HeapDockWidget::~HeapDockWidget()
{
    delete ui;
}
void HeapDockWidget::onAllocatorSelected(int index)
{
    if (index >= AllocatorCount)
        return;
    if (currentHeapWidget) {
        ui->verticalLayout->removeWidget(currentHeapWidget);
        delete currentHeapWidget;
    }
    if (index == Glibc) {
        currentHeapWidget = new GlibcHeapWidget(main, this);
    }
    ui->verticalLayout->addWidget(currentHeapWidget);
}
