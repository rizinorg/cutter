#ifndef ADDRESS_RANGE_SCROLLBAR_H
#define ADDRESS_RANGE_SCROLLBAR_H

#include "CutterCommon.h"

#include <QScrollBar>

/**
 * @brief Generic scrollbar class to be used by memory address based widgets (Disassembly, Hexdump
 * etc)
 */
class AddressRangeScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    AddressRangeScrollBar(QWidget *parent = nullptr);
    void refreshRange();
    void setPosition(RVA address);
    RVA address();

    RVA clampAddressToRange(RVA address) const;
    RVA rangeSize() const;

    /**
     * @brief Sends fake wheelEvent to the parent QScrollBar
     *
     * This allows external widgets (like side panels or text edits) to notify the
     * scrollbar of wheel activity. It is needed for systems (like MacOS) which have the option to
     * enable transient scrollbars meaning the ability to show the scrollbar only while scrolling
     * and hiding it later (when using Cutter's "Native" theme)
     */
    void showTransientScrollBar();
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
