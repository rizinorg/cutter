#include "MainWindow.h"
#include "CutterSeekableWidget.h"

CutterSeekableWidget::CutterSeekableWidget(QObject *parent)
 :
    QObject(parent)
{
    connect(Core(), &CutterCore::seekChanged, this, &CutterSeekableWidget::onSeekChanged);
}

void CutterSeekableWidget::onSeekChanged(RVA addr)
{
    if (isInSyncWithCore) {
        emit seekChanged(addr);
    }
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

void CutterSeekableWidget::toggleSyncWithCore()
{
    isInSyncWithCore = !isInSyncWithCore;
}

RVA CutterSeekableWidget::getIndependentOffset()
{
    return independentOffset;
}

RVA CutterSeekableWidget::getPrevIndependentOffset()
{
    return prevIdenpendentOffset;
}

bool CutterSeekableWidget::getSyncWithCore()
{
    return isInSyncWithCore;
}

void CutterSeekableWidget::setIndependentOffset(RVA addr)
{
    independentOffset = addr;
}

CutterSeekableWidget::~CutterSeekableWidget() {}