#ifndef HEXDUMPRANGEDIALOG_H
#define HEXDUMPRANGEDIALOG_H

#include "core/CutterCommon.h"
#include <QDialog>


namespace Ui {
class HexdumpRangeDialog;
}

class HexdumpRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HexdumpRangeDialog(QWidget *parent = nullptr, bool allowEmpty = false);
    ~HexdumpRangeDialog();
    bool empty();
    ut64 getStartAddress() const;
    ut64 getEndAddress() const;

    void    setStartAddress(ut64 start);
    void    open(ut64 start);

public slots:
    void textEdited();

private:
    bool getEndAddressRadioButtonChecked() const;
    bool getLengthRadioButtonChecked() const;
    bool validate();

    Ui::HexdumpRangeDialog *ui;
    bool emptyRange = true;
    ut64 startAddress;
    ut64 endAddress;
    bool allowEmpty = false;

private slots:
    void on_radioButtonClicked(bool checked);

};

#endif // HEXDUMPRANGEDIALOG_H
