#include "MainWindow.h"
#include "CutterSeekableWidget.h"

CutterSeekableWidget::CutterSeekableWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
}

void CutterSeekableWidget::seek(RVA addr)
{
    if (isInSyncWithCore) {
        Core()->seek(addr);
    }
    else {
        prevIdenpendentOffset = independentOffset;
        independentOffset = addr;
        emit seekChanged(addr);
    }
}

RVA CutterSeekableWidget::getOffset()
{
    RVA addr;
    if (isInSyncWithCore) {
        addr = Core()->getOffset();
    }
    else {
        addr = independentOffset;
    }
    return addr;
}

CutterSeekableWidget::~CutterSeekableWidget() {}