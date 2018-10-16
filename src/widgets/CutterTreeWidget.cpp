#include "CutterTreeWidget.h"
#include "MainWindow.h"

CutterTreeWidget::CutterTreeWidget(QObject *parent) :
    QObject(parent),
    bar(nullptr)
{}

void CutterTreeWidget::addStatusBar(QVBoxLayout *pos)
{
    if(!bar) {
        bar = new QStatusBar;
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        bar->setSizePolicy(sizePolicy);
        pos->addWidget(bar);
    }
}

void CutterTreeWidget::showItemsNumber(int count)
{
    if(bar){
        bar->showMessage(tr("%1 Items").arg(count));
    }
}

CutterTreeWidget::~CutterTreeWidget() {}
