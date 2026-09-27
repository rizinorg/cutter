#ifndef HEXDIFFSCROLLBAR_H
#define HEXDIFFSCROLLBAR_H

#include <QScrollBar>
#include <QWheelEvent>

#include <cstdint>

class HexDiffScrollBar : public QScrollBar
{
    Q_OBJECT

public:
    explicit HexDiffScrollBar(QWidget *parent = nullptr);

    void setAddressRange(quint64 begin, quint64 end);

    quint64 beginAddress() const;
    quint64 endAddress() const;
    quint64 address() const { return mAddress; }

    void setAddress(quint64 address);

    quint64 rangeSize() const;

    quint64 addressFromValue(int value) const;
    int valueFromAddress(quint64 address) const;

    void setAddressSingleStep(quint64 step);
    void setAddressPageStep(quint64 step);
signals:
    void addressChanged(quint64 address);

private slots:
    void onValueChanged(int value);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    quint64 mBeginAddress = 0;
    quint64 mEndAddress = 0;

    quint64 mAddressSingleStep = 0;
    quint64 mAddressPageStep = 0;

    quint64 mAddress = 0;
    void handleAction(int action);
    int accumScrollWheelDeltaY = 0;
};

#endif // HEXDIFFSCROLLBAR_H