#include "R2GraphWidget.h"
#include "ui_R2GraphWidget.h"

R2GraphWidget::R2GraphWidget(MainWindow *main)
    : CutterDockWidget(main)
    , ui(new Ui::R2GraphWidget)
    , graphView(new GenericR2GraphView(this, main))
{
    ui->setupUi(this);
    ui->verticalLayout->addWidget(graphView);
    connect(ui->refreshButton, &QPushButton::pressed, this, [this](){
        graphView->refreshView();
    });
}

R2GraphWidget::~R2GraphWidget()
{
}
