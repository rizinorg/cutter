#ifndef ADDRESS_RANGE_SCROLLBAR_H
#define ADDRESS_RANGE_SCROLLBAR_H

#include <QScrollBar>
#include "CutterCommon.h"

class AddressRangeScrollbar : public QScrollBar
{
    Q_OBJECT
public:
    AddressRangeScrollbar(QWidget *parent = nullptr);
    void refreshRange();
    bool setPosition(RVA address);
    RVA address();
    RVA rangeSize();

signals:
    void hideScrollbar();
    void showScrollbar();

private:
    RVA beginOffset = 0, endOffset = RVA_INVALID;
};

#endif // ADDRESS_RANGE_SCROLLBAR_H