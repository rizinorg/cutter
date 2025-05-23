#include "AddressRangeScrollbar.h"

void AddressRangeScrollbar::setRange(RVA newBeginOffset, RVA newEndOffset)
{
    beginOffset = newBeginOffset;
    endOffset = newEndOffset;
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