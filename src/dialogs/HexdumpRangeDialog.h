#ifndef HEXDUMPRANGEDIALOG_H
#define HEXDUMPRANGEDIALOG_H

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

private:
    Ui::HexdumpRangeDialog *ui;
};

#endif // HEXDUMPRANGEDIALOG_H
