#ifndef HEXDUMPRANGEDIALOG_H
#define HEXDUMPRANGEDIALOG_H

#include "core/CutterCommon.h" // IWYU pragma: keep

#include <QDialog>

#include <memory>

namespace Ui {
class HexdumpRangeDialog;
}

class HexdumpRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HexdumpRangeDialog(QWidget *parent = nullptr, bool allowEmpty = false);
    ~HexdumpRangeDialog();
    bool empty() const;
    RVA getStartAddress() const;
    RVA getEndAddress() const;

    void setStartAddress(ut64 start);
    void openAt(ut64 start);

public slots:
    void textEdited();

private:
    bool getEndAddressRadioButtonChecked() const;
    bool getLengthRadioButtonChecked() const;
    bool validate();

    std::unique_ptr<Ui::HexdumpRangeDialog> ui;
    bool emptyRange = true;
    RVA startAddress;
    RVA endAddress;
    bool allowEmpty = false;

private slots:
    void onRadioButtonClicked(bool checked);
};

#endif // HEXDUMPRANGEDIALOG_H
