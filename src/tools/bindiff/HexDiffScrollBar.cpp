#include "HexDiffScrollBar.h"

#include <QSignalBlocker>

#include <algorithm>
#include <limits>

HexDiffScrollBar::HexDiffScrollBar(QWidget *parent) : QScrollBar(Qt::Vertical, parent)
{
    setMinimum(0);
    setMaximum(0);

    connect(this, &QScrollBar::valueChanged, this, &HexDiffScrollBar::onValueChanged);
    connect(this, &QAbstractSlider::actionTriggered, this, &HexDiffScrollBar::handleAction);
}

void HexDiffScrollBar::setAddressRange(quint64 begin, quint64 end)
{
    mBeginAddress = begin;
    mEndAddress = end;
    if (rangeSize() > std::numeric_limits<int>::max()) {
        setRange(0, std::numeric_limits<int>::max());
    } else {
        setRange(0, rangeSize());
    }
    setSingleStep(0);
    setPageStep(0);
}

quint64 HexDiffScrollBar::addressFromValue(int value) const
{
    const quint64 range = mEndAddress - mBeginAddress;

    if (range == 0 || maximum() == 0) {
        return mBeginAddress;
    }

    const auto scaled = (static_cast<unsigned __int128>(range) * static_cast<quint64>(value))
            / static_cast<unsigned __int128>(maximum());

    return mBeginAddress + static_cast<quint64>(scaled);
}

int HexDiffScrollBar::valueFromAddress(quint64 address) const
{
    const quint64 range = mEndAddress - mBeginAddress;

    if (range == 0 || maximum() == 0) {
        return 0;
    }

    if (address <= mBeginAddress) {
        return 0;
    }

    if (address >= mEndAddress) {
        return maximum();
    }

    const quint64 offset = address - mBeginAddress;

    const auto scaled =
            (static_cast<unsigned __int128>(offset) * static_cast<unsigned __int128>(maximum()))
            / static_cast<unsigned __int128>(range);

    return static_cast<int>(scaled);
}

void HexDiffScrollBar::setAddress(quint64 newAddr)
{
    // const quint64 newAddr = std::clamp(addr, mBeginAddress, mEndAddress);//Not clampin followign
    // hex dumpview

    if (newAddr == mAddress) {
        return;
    }
    mAddress = newAddr;
    emit addressChanged(mAddress);
    const int newValue = valueFromAddress(mAddress);

    if (newValue == value()) {
        return;
    }

    setValue(newValue);
}

quint64 HexDiffScrollBar::beginAddress() const
{
    return mBeginAddress;
}

quint64 HexDiffScrollBar::endAddress() const
{
    return mEndAddress;
}

quint64 HexDiffScrollBar::rangeSize() const
{
    return mEndAddress - mBeginAddress;
}

void HexDiffScrollBar::onValueChanged(int value)
{
    if (valueFromAddress(mAddress) == value) {
        return;
    }
    mAddress = addressFromValue(value);
    emit addressChanged(mAddress);
}

void HexDiffScrollBar::handleAction(int action)
{
    switch (action) {
    case QAbstractSlider::SliderSingleStepAdd:
        setAddress(address() + mAddressSingleStep);
        break;

    case QAbstractSlider::SliderSingleStepSub:
        setAddress(address() - mAddressSingleStep);
        break;

    case QAbstractSlider::SliderPageStepAdd:
        setAddress(address() + mAddressPageStep);
        break;

    case QAbstractSlider::SliderPageStepSub:
        setAddress(address() - mAddressPageStep);
        break;

    default:
        break;
    }
}

void HexDiffScrollBar::wheelEvent(QWheelEvent *event)
{
    accumScrollWheelDeltaY += event->angleDelta().y();
    // Delta is reported in 1/8 of a degree
    // eg. 120 units * 1/8 = 15 degrees
    // Typical scroll speed is 1 line per 5 degrees
    const int lineDelta = 5 * 8;
    if (accumScrollWheelDeltaY >= lineDelta || accumScrollWheelDeltaY <= -lineDelta) {
        const int lineCount = accumScrollWheelDeltaY / lineDelta;
        accumScrollWheelDeltaY -= lineDelta * lineCount;
        if ((lineCount < 0 && address() == mBeginAddress)
            || (lineCount > 0 && address() == mEndAddress)) {
            return;
        }
        if (lineCount < 0) {
            setAddress(mAddress - std::abs(lineCount) * mAddressSingleStep);
        } else {
            setAddress(mAddress + std::abs(lineCount) * mAddressSingleStep);
        }
    }
}

void HexDiffScrollBar::setAddressSingleStep(quint64 step)
{
    mAddressSingleStep = step;
}
void HexDiffScrollBar::setAddressPageStep(quint64 step)
{
    mAddressPageStep = step;
}