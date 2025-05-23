#include "AddressRangeScrollbar.h"
#include "Cutter.h"

AddressRangeScrollbar::AddressRangeScrollbar(QWidget *parent) : QScrollBar(parent)
{
    connect(Core(), &CutterCore::refreshAll, this, &AddressRangeScrollbar::refreshRange);
}

void AddressRangeScrollbar::refreshRange()
{
    beginOffset = RVA_MAX;
    endOffset = 0;
    if (!Core()->currentlyEmulating && Core()->currentlyDebugging) {
        QString currentlyOpenFile = Core()->getConfig("file.path");
        QList<MemoryMapDescription> memoryMaps = Core()->getMemoryMap();
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
        RzPVector *mapsPtr = rz_io_maps(core->io);
        if (!mapsPtr) {
            emit hideScrollbar();
            return;
        }
        CutterPVector<RzIOMap> maps { mapsPtr };
        for (const RzIOMap *const map : maps) {
            // Skip the ESIL memory stack region
            if (Core()->currentlyEmulating && std::strncmp(rz_str_get(map->name), "mem.", 4) == 0) {
                continue;
            }
            ut64 b = rz_itv_begin(map->itv);
            ut64 e = rz_itv_end(map->itv);
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
        emit showScrollbar();
        return;
    }
    emit hideScrollbar();
    return;
}

bool AddressRangeScrollbar::setPosition(RVA address)
{
    const QSignalBlocker blocker(this);
    if (!maximum() || !rangeSize()) {
        return false;
    }
    int scrollBarPos = 0;
    if (address < beginOffset) {
        setValue(scrollBarPos);
        return true;
    }
    if (address > endOffset) {
        setValue(maximum());
        return true;
    }
    auto offset = address - beginOffset;
    if ((RVA_MAX / maximum()) < rangeSize()) {
        // Fallback formula for large files
        uint64_t smallBox = rangeSize() / maximum();
        uint64_t extra = rangeSize() % maximum();
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
    return true;
}

RVA AddressRangeScrollbar::address()
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

RVA AddressRangeScrollbar::rangeSize()
{
    return endOffset - beginOffset;
}