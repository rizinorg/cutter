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

void CutterSeekable::setSynchronization(bool sync)
{
    synchronized = sync;
    onCoreSeekChanged(Core()->getOffset());
    emit syncChanged();
}

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
    setSynchronization(!synchronized);
}

bool CutterSeekable::isSynchronized()
{
    return synchronized;
}

void CutterSeekable::seekToReference(RVA offset)
{
    if (offset == RVA_INVALID)
    {
        return;
    }
    RVA jump = Core()->getOffsetJump(offset);
    
    if (jump == RVA_INVALID) {
        QList<XrefDescription> refs = Core()->getXRefs(offset, false, false);
        
        if (refs.length()) {
            jump = refs.at(0).to;
        }

        if (refs.length() > 1) {
            qWarning() << "Too many references here. Weird behaviour expected.";
        }
    }

    if (jump != RVA_INVALID) {
        seek(jump);
    }
}