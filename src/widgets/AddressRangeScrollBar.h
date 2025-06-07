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

    RVA clampAddressToRange(RVA address);
    RVA rangeSize();

signals:
    void hideScrollBar();
    void showScrollBar();
    void scrolled(int lines);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    RVA beginOffset = 0, endOffset = RVA_INVALID;
    int accumScrollWheelDeltaY = 0;
};

#endif // ADDRESS_RANGE_SCROLLBAR_H