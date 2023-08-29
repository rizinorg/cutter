#include "core/MainWindow.h"
#include "CutterSeekable.h"

#include <QPlainTextEdit>

CutterSeekable::CutterSeekable(QObject *parent) : QObject(parent)
{
    connect(Core(), &CutterCore::seekChanged, this, &CutterSeekable::onCoreSeekChanged);
}

CutterSeekable::~CutterSeekable() {}

void CutterSeekable::setSynchronization(bool sync)
{
    synchronized = sync;
    onCoreSeekChanged(Core()->getOffset(), CutterCore::SeekHistoryType::New);
    emit syncChanged();
}

void CutterSeekable::onCoreSeekChanged(RVA addr, CutterCore::SeekHistoryType type)
{
    if (synchronized && widgetOffset != addr) {
        updateSeek(addr, type, true);
    }
}

void CutterSeekable::updateSeek(RVA addr, CutterCore::SeekHistoryType type, bool localOnly)
{
    previousOffset = widgetOffset;
    widgetOffset = addr;
    if (synchronized && !localOnly) {
        Core()->seek(addr);
    }

    emit seekableSeekChanged(addr, type);
}

void CutterSeekable::seekPrev()
{
    if (synchronized) {
        Core()->seekPrev();
    } else {
        this->seek(previousOffset, CutterCore::SeekHistoryType::Undo);
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
    if (offset == RVA_INVALID) {
        return;
    }

    QList<XrefDescription> refs = Core()->getXRefs(offset, false, false);

    if (refs.length()) {
        if (refs.length() > 1) {
            qWarning() << tr("More than one (%1) references here. Weird behaviour expected.")
                                  .arg(refs.length());
        }
        // Try first call
        for (auto &ref : refs) {
            if (ref.to != RVA_INVALID && ref.type == "CALL") {
                seek(ref.to);
                return;
            }
        }
        // Fallback to first valid, if any
        for (auto &ref : refs) {
            if (ref.to != RVA_INVALID) {
                seek(ref.to);
                return;
            }
        }
    }
}
