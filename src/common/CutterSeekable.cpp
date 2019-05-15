#include "core/MainWindow.h"
#include "CutterSeekable.h"

#include <QPlainTextEdit>


CutterSeekable::CutterSeekable(QObject *parent)
    :
    QObject(parent)
{
    connect(Core(), &CutterCore::seekChanged, this, &CutterSeekable::onCoreSeekChanged);
}

CutterSeekable::~CutterSeekable() {}

void CutterSeekable::onCoreSeekChanged(RVA addr)
{
    if (synchronized && widgetOffset != addr) {
        updateSeek(addr, true);
    }
}

void CutterSeekable::updateSeek(RVA addr, bool localOnly)
{
    previousOffset = widgetOffset;
    widgetOffset = addr;
    if (synchronized && !localOnly) {
        Core()->seek(addr);
    }

    emit seekableSeekChanged(addr);
}


void CutterSeekable::seekPrev()
{
    if (synchronized) {
        Core()->seekPrev();
    } else {
        this->seek(previousOffset);
    }
}

RVA CutterSeekable::getOffset()
{
    return (synchronized) ? Core()->getOffset() : widgetOffset;
}

void CutterSeekable::toggleSynchronization()
{
    synchronized = !synchronized;
    onCoreSeekChanged(Core()->getOffset());
}

bool CutterSeekable::isSynchronized()
{
    return synchronized;
}
