#ifndef ADDRESS_RANGE_SCROLLBAR_H
#define ADDRESS_RANGE_SCROLLBAR_H

#include <QScrollBar>
#include "CutterCommon.h"

class AddressRangeScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    AddressRangeScrollBar(QWidget *parent = nullptr);
    void refreshRange();
    void setPosition(RVA address);
    RVA address();
    RVA rangeSize();

signals:
    void hideScrollBar();
    void showScrollBar();

private:
    RVA beginOffset = 0, endOffset = RVA_INVALID;
};

#endif // ADDRESS_RANGE_SCROLLBAR_H