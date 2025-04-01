#include "HeapDockWidget.h"
#include "ui_HeapDockWidget.h"
#include "widgets/GlibcHeapWidget.h"
#include "widgets/WindowsHeapWidget.h"
#include "AsyncTask.h"
#include <QPointer>  // For QPointer safety


HeapDockWidget::HeapDockWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::HeapDockWidget), main(main)
{
    ui->setupUi(this);

    ui->allocatorSelector->addItem("Glibc Heap");
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);

    connect<void (QComboBox::*)(int)>(ui->allocatorSelector, &QComboBox::currentIndexChanged, this,
                                      &HeapDockWidget::onAllocatorSelected);

    // select Glibc heap by default
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

    // remove the current heap widget from layout
    if (currentHeapWidget) {
        ui->verticalLayout->removeWidget(currentHeapWidget);
        delete currentHeapWidget;
    }

    // change widget depending upon selected allocator
    if (index == Glibc) {
        currentHeapWidget = new GlibcHeapWidget(main, this);
    }
    else if (index == Windows) { // Add this new case
        // Create empty widget first
        currentHeapWidget = new WindowsHeapWidget(main, this);
        
        // Load data asynchronously
        m_asyncTask = new AsyncTask([this]() {
            if (!core) return (RzList*)nullptr;
            return rz_core_get_windows_heaps(core);  // Background task
        });
        connect(m_asyncTask, &AsyncTask::finished, 
                dynamic_cast<WindowsHeapWidget*>(currentHeapWidget), 
                &WindowsHeapWidget::updateData);
        connect(m_asyncTask, &AsyncTask::error, this, [](){ 
            qWarning() << "Failed to load Windows heaps"; 
             });
        m_asyncTask->start();
    }
    ui->verticalLayout->addWidget(currentHeapWidget);
}
