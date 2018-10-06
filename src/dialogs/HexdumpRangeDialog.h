#ifndef HEXDUMPRANGEDIALOG_H
#define HEXDUMPRANGEDIALOG_H

#include "Cutter.h"

#include <QDialog>

namespace Ui {
class HexdumpRangeDialog;
}

class HexdumpRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HexdumpRangeDialog(QWidget *parent = nullptr);
    ~HexdumpRangeDialog();
    QString getStartAddress();
    QString getEndAddress();
    QString getLength();
    bool    getEndAddressRadioButtonChecked();
    bool    getLengthRadioButtonChecked();
    void    setStartAddress(ut64 start);

public slots:
    void textEdited();

private:
    Ui::HexdumpRangeDialog *ui;

private slots:
    void on_radioButtonClicked(bool checked);

};

#endif // HEXDUMPRANGEDIALOG_H
