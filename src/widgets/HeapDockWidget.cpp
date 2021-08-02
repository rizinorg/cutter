#include "HeapDockWidget.h"
#include "ui_HeapDockWidget.h"
#include "widgets/GlibcHeapWidget.h"
#include "WindowsHeapWidget.h"

HeapDockWidget::HeapDockWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::HeapDockWidget), main(main)
{
    ui->setupUi(this);

    ui->allocatorSelector->addItem("Glibc Heap");
    ui->allocatorSelector->addItem("Windows Heap");
    ui->verticalLayout->setMargin(0);

    connect<void (QComboBox::*)(int)>(ui->allocatorSelector, &QComboBox::currentIndexChanged, this,
                                      &HeapDockWidget::onAllocatorSelected);

    // select default heap depending upon kernel type
    if (QSysInfo::kernelType() == "linux") {
        ui->allocatorSelector->setCurrentIndex(Glibc);
        onAllocatorSelected(Glibc);
    } else if (QSysInfo::kernelType() == "winnt") {
        ui->allocatorSelector->setCurrentIndex(Windows);
        onAllocatorSelected(Windows);
    }
}

HeapDockWidget::~HeapDockWidget()
{
    delete ui;
}

void HeapDockWidget::onAllocatorSelected(int index)
{
    if (index >= AllocatorCount)
        return;

    // remove the current heap widget from layout
    if (currentHeapWidget) {
        ui->verticalLayout->removeWidget(currentHeapWidget);
        delete currentHeapWidget;
    }

    // change widget depending upon selected allocator
    if (index == Glibc) {
        currentHeapWidget = new GlibcHeapWidget(main, this);
    } else if (index == Windows) {
        currentHeapWidget = new WindowsHeapWidget(main, this);
    }

    ui->verticalLayout->addWidget(currentHeapWidget);
}
