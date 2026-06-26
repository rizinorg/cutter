#include "AddressRangeScrollBar.h"

#include "Cutter.h"

#include <QWheelEvent>

#include <algorithm>
#include <cstring>

AddressRangeScrollBar::AddressRangeScrollBar(QWidget *parent) : QScrollBar(parent)
{
    connect(Core(), &CutterCore::refreshAll, this, &AddressRangeScrollBar::refreshRange);
    connect(this, &AddressRangeScrollBar::actionTriggered, this, [this](int action) {
        switch (action) {
        // Due to the way the QScrollBar::actionTriggered signal works,
        // setting the slider pos to its current value here
        // prevents it from moving, allowing us to basically
        // override behavior for specific actions
        // See https://doc.qt.io/qt-6/qabstractslider.html#actionTriggered
        // for more info.
        case QAbstractSlider::SliderSingleStepAdd:
            setSliderPosition(value());
            if (value() == maximum()) {
                return;
            }
            emit scrolled(-singleStep());
            return;
        case QAbstractSlider::SliderPageStepAdd:
            setSliderPosition(value());
            if (value() == maximum()) {
                return;
            }
            emit scrolled(-pageStep());
            return;
        case QAbstractSlider::SliderSingleStepSub:
            setSliderPosition(value());
            if (value() == minimum()) {
                return;
            }
            emit scrolled(singleStep());
            return;
        case QAbstractSlider::SliderPageStepSub:
            setSliderPosition(value());
            if (value() == minimum()) {
                return;
            }
            emit scrolled(pageStep());
            return;
        default:
            return;
        }
    });
}

void AddressRangeScrollBar::refreshRange()
{
    beginOffset = RVA_MAX;
    endOffset = 0;
    if (!Core()->currentlyEmulating && Core()->currentlyDebugging) {
        const QString currentlyOpenFile = Core()->getConfig("file.path");
        const QList<MemoryMapDescription> memoryMaps = Core()->getMemoryMap();
        for (const MemoryMapDescription &map : memoryMaps) {
            if (map.fileName == currentlyOpenFile) {
                if (map.addrStart < beginOffset) {
                    beginOffset = map.addrStart;
                }
                if (map.addrEnd > endOffset) {
                    endOffset = map.addrEnd;
                }
            }
        }
    } else {
        RzCoreLocked core(Core());
        const RzPVector *mapsPtr = rz_io_maps(core->io);
        if (!mapsPtr) {
            emit hideScrollBar();
            return;
        }
        const CutterPVector<RzIOMap> maps { mapsPtr };
        for (const RzIOMap *const map : maps) {
            // Skip the ESIL memory stack region
            if (Core()->currentlyEmulating && std::strncmp(rz_str_get(map->name), "mem.", 4) == 0) {
                continue;
            }
            const ut64 b = rz_itv_begin(map->itv);
            const ut64 e = rz_itv_end(map->itv);
            if (b < beginOffset) {
                beginOffset = b;
            }
            if (e > endOffset) {
                endOffset = e;
            }
        }
    }
    if (endOffset) {
        --endOffset;
    }
    if (endOffset == 0) {
        beginOffset = 0;
    }
    setMinimum(0);
    // Increasing this value increases scroll bar accuracy for small files but
    // decreases it for large files
    // Sufficiently below 2^32 to avoid causing problems in calculations done by QScrollbar,
    // otherwise as high as possible to maximize range in which address map 1:1 to scrollbar pos.
    const int rangeMax = 512 * 1024 * 1024;
    if (rangeSize() > rangeMax) {
        setMaximum(rangeMax);
    } else {
        setMaximum(rangeSize());
    }
    if (rangeSize()) {
        emit showScrollBar();
        return;
    }
    emit hideScrollBar();
    return;
}

void AddressRangeScrollBar::setPosition(RVA address)
{
    const QSignalBlocker blocker(this);
    if (!maximum() || !rangeSize()) {
        emit hideScrollBar();
        return;
    }
    int scrollBarPos = 0;
    if (address < beginOffset) {
        setValue(scrollBarPos);
        emit showScrollBar();
        return;
    }
    if (address > endOffset) {
        setValue(maximum());
        emit showScrollBar();
        return;
    }
    auto offset = address - beginOffset;
    if ((RVA_MAX / maximum()) < rangeSize()) {
        // Fallback formula for large files
        const uint64_t smallBox = rangeSize() / maximum();
        const uint64_t extra = rangeSize() % maximum();
        auto bigBoxRange = (smallBox + 1) * extra;
        if (offset < bigBoxRange) {
            scrollBarPos = offset / (smallBox + 1);
        } else {
            scrollBarPos = extra + (offset - bigBoxRange) / smallBox;
        }
    } else {
        scrollBarPos = (maximum() * offset) / rangeSize();
    }
    if (address != beginOffset && scrollBarPos == 0) {
        scrollBarPos = 1;
    }
    setValue(scrollBarPos);
    emit showScrollBar();
    return;
}

RVA AddressRangeScrollBar::address()
{
    if (!maximum() || !rangeSize()) {
        return beginOffset;
    }
    // Fallback formula for large files
    if ((RVA_MAX / maximum()) < rangeSize()) {
        return value() * (rangeSize() / maximum()) + std::min<RVA>(value(), rangeSize() % maximum())
                + beginOffset;
    }
    return (value() * rangeSize()) / maximum() + beginOffset;
}

RVA AddressRangeScrollBar::clampAddressToRange(RVA address) const
{
    if (address > endOffset) {
        return endOffset;
    }
    if (address < beginOffset) {
        return beginOffset;
    }
    return address;
}

RVA AddressRangeScrollBar::rangeSize() const
{
    return endOffset - beginOffset;
}

void AddressRangeScrollBar::showTransientScrollBar()
{
    const Qt::ScrollPhase phases[] = { Qt::ScrollBegin, Qt::ScrollEnd };
    for (const auto &phase : phases) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QWheelEvent event(QPointF(0, 0), QPointF(0, 0), QPoint(0, 0), QPoint(0, 0), Qt::NoButton,
                          Qt::NoModifier, phase, false);
#else
        QWheelEvent event(QPointF(0, 0), QPointF(0, 0), QPoint(0, 0), QPoint(0, 0), 0, Qt::Vertical,
                          Qt::NoButton, Qt::NoModifier, phase);
#endif
        QScrollBar::wheelEvent(&event);
    }
}

void AddressRangeScrollBar::wheelEvent(QWheelEvent *event)
{
    accumScrollWheelDeltaY += event->angleDelta().y();
    // Delta is reported in 1/8 of a degree
    // eg. 120 units * 1/8 = 15 degrees
    // Typical scroll speed is 1 line per 5 degrees
    const int lineDelta = 5 * 8;
    if (accumScrollWheelDeltaY >= lineDelta || accumScrollWheelDeltaY <= -lineDelta) {
        const int lineCount = accumScrollWheelDeltaY / lineDelta;
        accumScrollWheelDeltaY -= lineDelta * lineCount;
        if ((lineCount < 0 && value() == maximum()) || (lineCount > 0 && value() == minimum())) {
            return;
        }
        emit scrolled(lineCount);
    }
}
