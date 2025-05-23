#ifndef ADDRESS_RANGE_SCROLLBAR_H
#define ADDRESS_RANGE_SCROLLBAR_H

#include <QScrollBar>
#include "CutterCommon.h"

class AddressRangeScrollbar : public QScrollBar
{
    Q_OBJECT
public:
    using QScrollBar::QScrollBar;
    void setRange(RVA newBeginOffset, RVA newEndOffset);
    bool setPosition(RVA address);
    RVA address();
    RVA rangeSize();

private:
    RVA beginOffset = 0, endOffset = RVA_INVALID;
};

#endif // ADDRESS_RANGE_SCROLLBAR_H