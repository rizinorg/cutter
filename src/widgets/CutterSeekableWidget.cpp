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

// void CutterSeekableWidget::toggleSync(QString windowTitle, void (* callback)(RVA))
// {
//     isInSyncWithCore = !isInSyncWithCore;
//     if (isInSyncWithCore) {
//         setWindowTitle(windowTitle);
//         connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(callback(RVA)));
//     }
//     else {
//         setWindowTitle(windowTitle + " (not synced)");
//         independentOffset = Core()->getOffset();
//         disconnect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(callback(RVA)));
//     }
// }

CutterSeekableWidget::~CutterSeekableWidget() {}