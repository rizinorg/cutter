#include "WindowsHeapWidget.h"
#include "ui_WindowsHeapWidget.h"

WindowsHeapWidget::WindowsHeapWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WindowsHeapWidget)
{
    ui->setupUi(this);
}

WindowsHeapWidget::~WindowsHeapWidget()
{
    delete ui;
}
